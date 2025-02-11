#include <stdio.h>
#include <math.h>

//JAYDEN YOU ARE ON "Material Controls"
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
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

ew::Camera camera;	//our camera
ew::CameraController cameraController;
ew::Transform monkeyTransform;
ew::Mesh plane;

//for material controls:
struct Material 
{
	float Ka = 1.0;
	float Kd = 0.5;
	float Ks = 0.5;
	float Shininess = 128;
}material;

struct Debug
{
	//glm::vec3 color = glm::vec3{0.2627, 0.31f, 0.85f};
	glm::vec3 color = glm::vec3{0.00f, 0.31f, 0.85f};

	float sample1Offset = (1.0);
	float sample2Offset = (0.2);
	float uniformStrength = 10.0f;
	float tilling = 1.0f;
	float b1 = 0.9;
	float b2 = 0.02;
	float scale = 10.0f;

	float warpScale = 1.0f;
	float warpStrength = 0.2f;
	float specScale = 1.0f;

	float brightness_lower_cutoff = 0.2;
	float brightness_upper_cutoff = 9.0;
}debug;

int main() {
	GLFWwindow* window = initWindow("WorkSesh1", screenWidth, screenHeight);

	ew::Shader newShader = ew::Shader("assets/water.vert", "assets/water.frag");	//our shader
	GLuint waterTexture0 = ew::loadTexture("assets/doubledash/wave_tex.png"); 
	GLuint waterTexture1 = ew::loadTexture("assets/doubledash/wave_spec.png");
	GLuint waterTexture2 = ew::loadTexture("assets/doubledash/wave_warp.png");

	camera.position = glm::vec3(0.0f, 5.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f);	//look at center of scene
	camera.aspectRatio = (float)screenWidth / screenHeight; 
	camera.fov = 60.0f;								//vertical field of view in degrees

	//pipeline definition
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	plane.load(ew::createPlane(50.0f, 50.0f, 100));
	glCheckError();

	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	while (!glfwWindowShouldClose(window)) 
	{
		glfwPollEvents();
		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;
		cameraController.move(window, &camera, deltaTime);

		//RENDER
		glClearColor(0.6f,0.8f,0.92f,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);

		newShader.use();

		glActiveTexture(GL_TEXTURE0);
		glBindTextureUnit(0, waterTexture0);
		glBindTextureUnit(1, waterTexture1);
		glBindTextureUnit(2, waterTexture2);

		newShader.setInt("wave_tex", 0);
		newShader.setInt("wave_spec", 1);
		newShader.setInt("wave_warp", 2);

		newShader.setFloat("tilling", debug.tilling);
		newShader.setFloat("time", time);
		newShader.setFloat("uniform_strength", debug.uniformStrength);
		newShader.setFloat("scale", debug.scale);

		newShader.setFloat("sampler1Offset", debug.sample1Offset);
		newShader.setFloat("sampler2Offset", debug.sample2Offset);
		newShader.setFloat("b1", debug.b1);
		newShader.setFloat("b2", debug.b2);

		newShader.setFloat("warp_scale", debug.warpScale);
		newShader.setFloat("warp_strength", debug.warpStrength);
		newShader.setFloat("spec_scale", debug.specScale);

		newShader.setFloat("brightness_lower_cutoff", debug.brightness_lower_cutoff);
		newShader.setFloat("brightness_upper_cutoff", debug.brightness_upper_cutoff);

		newShader.setMat4("_Model", glm::mat4(1.0f));
		newShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());

		newShader.setVec3("waterColor", debug.color);
		newShader.setVec3("cameraPos", camera.position);

		plane.draw();
		glCheckError();

		//draw UI
		drawUI();
		
		//swap buffers (swap wahatever on screen, and whatever we write to)
		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}

void resetCamera(ew::Camera* camera, ew::CameraController* controller)
{
	//reset position
	camera->position = glm::vec3(0, 0, 5.0f);
	//reset target
	camera->target = glm::vec3(0);

	//reset controller rotation
	controller->yaw = controller->pitch = 0;
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
		ImGui::SliderFloat("Ambient K", &material.Ka, 0.0f, 1.0f);
		ImGui::SliderFloat("Diffuse K", &material.Kd, 0.0f, 1.0f);
		ImGui::SliderFloat("Specular K", &material.Ks, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &material.Shininess, 2.0f, 1024.0f);
	}
	ImGui::ColorEdit3("Water Color", &debug.color[0]);
	ImGui::SliderFloat("Tilling", &debug.tilling, 1.0f, 10.0f);
	ImGui::SliderFloat("Blend 1", &debug.b1, 0.0f, 1.0f);
	ImGui::SliderFloat("Blend 2", &debug.b2, 0.0f, 1.0f);
	ImGui::SliderFloat("Samp 1 Offset", &debug.sample1Offset, 0.0f, 5.0f);
	ImGui::SliderFloat("Samp 2 Offset", &debug.sample2Offset, 0.0f, 5.0f);
	ImGui::SliderFloat("Scale", &debug.scale, 0.0f, 50.0f);
	ImGui::SliderFloat("Uniform Strength", &debug.uniformStrength, 0.0f, 20.0f);

	ImGui::SliderFloat("Warp Scale", &debug.warpScale, 0.0f, 10.0f);
	ImGui::SliderFloat("Spec Scale", &debug.specScale, 0.0f, 10.0f);
	ImGui::SliderFloat("Warp Strength", &debug.warpStrength, 0.0f, 10.0f);

	ImGui::SliderFloat("Brightness Lower", &debug.brightness_lower_cutoff, 0.0f, 10.0f);
	ImGui::SliderFloat("Brightness Upper", &debug.brightness_upper_cutoff, 0.0f, 10.0f);

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
