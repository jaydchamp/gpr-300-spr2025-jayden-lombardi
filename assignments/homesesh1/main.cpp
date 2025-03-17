#include <stdio.h>
#include <math.h>

//JAYDEN YOU ARE ON "Material Controls"
#include <ew/external/glad.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ew/procGen.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/transform.h>
#include <ew/cameraController.h>
#include <ew/texture.h>

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();
void initCamera();
void definePipline();
void render(ew::Shader shader, ew::Shader lightPassShader, ew::Shader lightVShader, ew::Model model, GLuint texture);
void post_process(ew::Shader& shader);
#define glCheckError() glCheckError_(__FILE__, __LINE__)

//Global state
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

ew::Camera camera;	//our camera
ew::CameraController cameraController;
ew::Transform monkeyTransform;
ew::Transform lightSphereTransform;
ew::Mesh sphere; //light sphere

static float quad_vertices[] = {
	// pos (x, y) texcoord (u, v)
	-1.0f,  1.0f, 0.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 0.0f,
	1.0f, -1.0f, 1.0f, 0.0f,

	-1.0f,  1.0f, 0.0f, 1.0f,
	1.0f, -1.0f, 1.0f, 0.0f,
	1.0f,  1.0f, 1.0f, 1.0f,
};

struct FullScreenQuad
{
	GLuint vao;
	GLuint vbo;

	void Initalize()
	{
		//inint full screen quad
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0); // pos
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1); // texcoords
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(sizeof(float) * 2));

		glBindVertexArray(0);
	}
}fullscreen_quad;

struct Material
{
	glm::vec3 ambientK = glm::vec3(0.4);
	glm::vec3 diffuseK = glm::vec3(0.5);
	glm::vec3 specularK = glm::vec3(0.5);
	float shininess = 128;
}materials;

struct FrameBuffer
{
	GLuint fbo;
	GLuint depthTexture;
	GLuint color;

	GLuint position;
	GLuint normal;
	GLuint lighting;
	GLuint lights;

	GLuint color1;
	GLuint color2;


	void Initialize()
	{
		//bind buffer
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		//color attachment
		glGenTextures(1, &color);
		glBindTexture(GL_TEXTURE_2D, color);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

		//position attachment
		glGenTextures(1, &position);
		glBindTexture(GL_TEXTURE_2D, position);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, position, 0);

		//normal attachment
		glGenTextures(1, &normal);
		glBindTexture(GL_TEXTURE_2D, normal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, normal, 0);

		//lighting attachment
		glGenTextures(1, &lighting);
		glBindTexture(GL_TEXTURE_2D, lighting);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, lighting, 0);

		//LIGHTS attachment
		glGenTextures(1, &lights);
		glBindTexture(GL_TEXTURE_2D, lights);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, lights, 0);

		//draw all 5 buffers
		GLuint arr[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
		glDrawBuffers(5, arr);

		//depth attachment
		glGenTextures(1, &depthTexture);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, screenWidth, screenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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

}frameBuffer;

int main() {
	GLFWwindow* window = initWindow("Work Sesh 04", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	ew::Shader defaultShader = ew::Shader("assets/default.vert", "assets/default.frag");	//our shader
	ew::Shader lightPassShader = ew::Shader("assets/lightPass.vert", "assets/lightPass.frag");	//our shader
	ew::Shader lightVShader = ew::Shader("assets/lightV.vert", "assets/lightV.frag");	//our shader
	ew::Shader geoShader = ew::Shader("assets/geo.vert", "assets/geo.frag");	//our shader

	ew::Model monkeyModel = ew::Model("assets/suzanne.obj");					//our model

	GLuint brickTexture = ew::loadTexture("assets/brick_color.jpg");
	//Normal Mapping?

	initCamera();

	frameBuffer.Initialize();
	fullscreen_quad.Initalize();
	sphere.load(ew::createSphere(0.5f, 4));

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		cameraController.move(window, &camera, deltaTime);
		render(defaultShader, lightPassShader, lightVShader, monkeyModel, brickTexture);
		post_process(geoShader);

		drawUI();
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

void initCamera()
{
	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f);	//look at center of scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f;
	camera.farPlane = 1000;
}

void render(ew::Shader defShader, ew::Shader lightPassShader, ew::Shader lightVShader, ew::Model model, GLuint texture)
{
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.fbo);
	{

		definePipline();

		defShader.use();
		defShader.setMat4("_Model", monkeyTransform.modelMatrix());

		for (auto i = 0; i <= 10; i++)
		{
			for (auto j = 0; j <= 10; j++)
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTextureUnit(0, texture);

				//monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));
				defShader.setMat4("_ViewProj", camera.projectionMatrix() * camera.viewMatrix());
				defShader.setInt("_MainTexture", 0);
				//defShader.setInt("_NormalMap", 0);
				defShader.setMat4("_Model", glm::translate(glm::vec3(i * 2.0f, 0.0f, j * 2.0f)));
				model.draw();
			}
		}

	}
	//render lights
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.fbo);
	{
		definePipline();

		//textures
		for (int i = 0; i <= 10; i++)
		{
			for (int j = 0; j <= 10; j++)
			{
				lightVShader.use();
				lightVShader.setMat4("_CameraViewProj", camera.projectionMatrix() * camera.viewMatrix());
				lightVShader.setVec3("_Color", glm::vec3(1.0, 0.0, 0.0));
				lightVShader.setMat4("_Model", glm::translate(glm::vec3(i * 2.0f, 5, j * 2.0f)));
				sphere.draw();

				glBindVertexArray(fullscreen_quad.vao);
				glDisable(GL_DEPTH_TEST);

				//bind color texture
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, frameBuffer.color);

				//bind position texture
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, frameBuffer.position);

				//bind normal texture
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, frameBuffer.normal);

				//bind lighting texture
				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_2D, frameBuffer.lighting);

				lightPassShader.use();
				lightPassShader.setInt("_PositionTex", 1);
				lightPassShader.setInt("_NormalTex", 2);
				lightPassShader.setInt("_PrevLightPass", 3);
				lightPassShader.setVec3("_CamPos", camera.position);

				lightPassShader.setVec3("_Light.pos", glm::vec3(i * 2.0f, 5, j * 2.0f));
				//lightPassShader.setVec3("_Light.color", glm::vec3(i * 2.0f, 5, j * 2.0f));

				lightPassShader.setVec3("_Material.ambientK", materials.ambientK);
				lightPassShader.setVec3("_Material.diffuseK", materials.diffuseK);
				lightPassShader.setVec3("_Material.specularK", materials.specularK);
				lightPassShader.setFloat("_Material.shininess", materials.shininess);

				glDrawArrays(GL_TRIANGLES, 0, 6);
			}
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void post_process(ew::Shader& shader)
{
	glBindVertexArray(fullscreen_quad.vao);
	{
		glDisable(GL_DEPTH_TEST);
		glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, frameBuffer.color);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, frameBuffer.lighting);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, frameBuffer.lights);

		shader.use();
		shader.setInt("_Albedo", 0);
		shader.setInt("_LightingTexture", 1);
		shader.setInt("_Lights", 2);

		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	glBindVertexArray(0);
}

void definePipline()
{
	//pipeline definition
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClear(GL_DEPTH_BUFFER_BIT);
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

	//ImGui::Image(intptr_t(frameBuffer.color0));

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