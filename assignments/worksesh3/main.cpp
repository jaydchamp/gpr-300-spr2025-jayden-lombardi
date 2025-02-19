#include <stdio.h>
#include <math.h>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/transform.hpp"
#include <ew/external/glad.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ew/procGen.h>

#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/transform.h>
#include <ew/cameraController.h>
#include <ew/texture.h>
#include <iostream>

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();
void initCamera();
void definePipline();
void render(ew::Shader shader, ew::Model model, GLuint texture);
GLenum glCheckError_(const char* file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__) 

//Global state
#define SHADOW_MAP_SIZE (1024)
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

ew::Camera camera;	//our camera
ew::CameraController cameraController;
ew::Transform monkeyTransform;
ew::Mesh plane;
static glm::vec4 light_orbit_radius = {2.0f, 2.0f, -2.0f, 1.0f};

//for material controls:
struct Material 
{
	glm::vec3 ambient = glm::vec3(1.0);
	glm::vec3 diffuse = glm::vec3(0.5);
	glm::vec3 specular = glm::vec3(0.5);
	float shininess = 128;
}material;

struct Light
{
	glm::vec3 color;
	glm::vec3 position;
}light;

struct Ambient
{
	glm::vec3 color;
	glm::vec3 direction;
	float intensity;
}ambient;

struct Debug
{
	glm::vec3 color = glm::vec3{ 0.00f, 0.31f, 0.85f };
	float bias = 0.005f;
	bool cull_front = false;
	bool use_pcf = false;
}debug;

struct DepthBuffer
{
	GLuint fbo;
	GLuint depthTexture;
	
	void Initialize()
	{
		glGenFramebuffers(1, &fbo);
		glGenTextures(1, &depthTexture);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 256, 256, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);

			GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
			{
				printf("Framebuffer incomplete: %d", fboStatus);
			}
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

}depthBuffer;

int main() 
{
	GLFWwindow* window = initWindow("Assignment 0", screenWidth, screenHeight);

	//shaders
	ew::Shader newShader = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader shadow_pass = ew::Shader("assets/shadow_pass.vert", "assets/shadow_pass.frag");
	ew::Shader blinnPhongShader = ew::Shader("assets/blinnphong.vert", "assets/blinnphong.frag");

	//model + texture
	ew::Model monkeyModel = ew::Model("assets/suzanne.obj");			
	GLuint brickTexture = ew::loadTexture("assets/brick_color.jpg");

	initCamera();
	definePipline();

	plane.load(ew::createPlane(50.0f, 50.0f, 100));

	light.position = glm::vec3(1.0f);
	light.color = glm::vec3(0.5f, 0.5f, 0.5f); 
	ambient.intensity = 1.0f;
	ambient.color = glm::vec3(0.5f, 0.5f, 0.5f);

	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback); 

	depthBuffer.Initialize();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		render(blinnPhongShader, monkeyModel, brickTexture);
		cameraController.move(window, &camera, deltaTime);


		drawUI();

		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}

//jayden added
void resetCamera(ew::Camera* camera, ew::CameraController* controller)
{
	//reset position
	camera->position = glm::vec3(0, 0, 5.0f);
	//reset target
	camera->target = glm::vec3(0);

	//reset controller rotation
	controller->yaw = controller->pitch = 0;
}

void initCamera()
{
	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f);	//look at center of scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f;
}

void render(ew::Shader shader, ew::Model model, GLuint texture)
{
	float time = (float)glfwGetTime();
	deltaTime = time - prevFrameTime;
	prevFrameTime = time;

	const auto rym = glm::rotate((float)time/*.absolute*/, glm::vec3(0.0f, 1.0f, 0.0f));
	light.position = rym * light_orbit_radius;

	const auto camera_view_proj  = camera.projectionMatrix() * camera.viewMatrix();
	const auto light_projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
	const auto light_view = glm::lookAt(light.position, glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	const auto light_view_proj = light_projection * light_view;

	//shadow pass
	glBindFramebuffer(GL_FRAMEBUFFER, depthBuffer.fbo);
	{
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

		//begin pass
		glClear(GL_DEPTH_BUFFER_BIT);

		shadow_pass.use();
		shadow_pass.setMat4("_Model", glm::mat4(1.0f));
		shadow_pass.setMat4("_LightViewProjection", light_view_proj);
		model.draw();

		glCheckError();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//render lighting
	{
		glViewport(0, 0, screenWidth, screenHeight);
		 
		glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthBuffer.depthTexture);

		shader.use();

		//samplers
		shader.setInt("shadow_map", 0);

		//scene matrices
		shader.setMat4("_Model", glm::mat4(1.0f));
		shader.setMat4("_CameraViewProjection", camera_view_proj);
		shader.setMat4("_LightViewProjection", light_view_proj);
		shader.setVec3("camera_pos", camera.position);
		shader.setVec3("light_pos", light.position);

		//material properties
		shader.setVec3("_Material.ambient", material.ambient);
		shader.setVec3("_Material.diffuse", material.diffuse);
		shader.setVec3("_Material.specular", material.specular);
		shader.setFloat("_Material.shininess", material.shininess);

		//ambient light
		shader.setVec3("_Ambient.color", ambient.color);
		shader.setFloat("_Ambient.intensity", ambient.intensity);

		//point light
		shader.setVec3("_Light.color", light.color);
		shader.setVec3("_Light.position", light.position);

		shader.setFloat("bias", debug.bias);
		shader.setInt("use_pcf", debug.use_pcf);

		model.draw();

		shader.setMat4("_Model", glm::translate(glm::vec3(0.0f, -2.0f, 0.0f)));

		plane.draw();
	}
}

void definePipline()
{
	//pipeline definition
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void drawUI() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");
	if (ImGui::Button("Reset Camera"))
	{
		resetCamera(&camera, &cameraController);
	}
	if (ImGui::CollapsingHeader("Material")) 
	{
		//ImGui::SliderFloat("Ambient K", &material.ambient, 0.0f, 1.0f);
		//ImGui::SliderFloat("Diffuse K", &material.diffuse, 0.0f, 1.0f);
		//ImGui::SliderFloat("Specular K", &material.specular, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &material.shininess, 2.0f, 1024.0f);
	}

	//ImGui::Separator("Depth Image");
	ImGui::Separator(); //depth image
	ImGui::Image((ImTextureID)(intptr_t)depthBuffer.depthTexture, ImVec2(256, 256));

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenWidth = width;
	screenHeight = height;
	camera.aspectRatio = (float)screenWidth / screenHeight;
}

/// <summary>
/// Initializes GLFW, GLAD, and IMGUI
/// </summary>
/// <param name="title">Window title</param>
/// <param name="width">Window width</param>
/// <param name="height">Window height</param>
/// <returns>Returns window handle on success or null on fail</returns>
GLFWwindow* initWindow(const char* title, int width, int height) {
	printf("Initializing...");
	if (!glfwInit()) {
		printf("GLFW failed to init!");
		return nullptr;
	}

	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		printf("GLFW failed to create window");
		return nullptr;
	}
	//whenever you init you need to send current context
	glfwMakeContextCurrent(window);

	//when we load the proc address, computer if figuring out where these function calls live on the GPU
	if (!gladLoadGL(glfwGetProcAddress)) {
		printf("GLAD Failed to load GL headers");
		return nullptr;
	}

	//Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return window;
}