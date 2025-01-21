#include <stdio.h>
#include <math.h>

//JAYDEN YOU ARE ON "Material Controls"
#include <ew/external/glad.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/transform.h>
#include <ew/cameraController.h>
#include <ew/texture.h>
#include <ew/external/stb_image.h>
#include <tuple>

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();

//Global state
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;


ew::Camera camera;	//our camera
ew::CameraController cameraController;
ew::Transform monkeyTransform;

//for material controls:
struct Material 
{
	float Ka = 1.0;
	float Kd = 0.5;
	float Ks = 0.5;
	float Shininess = 128;
}material;

typedef struct {
	glm::vec3 highlight;
	glm::vec3 shadow;
} Palette;


//struct Palette{
//	glm::vec3 highlight;
//	glm::vec3 shadow;
//}palette;

static int palette_index = 0;
static std::vector<std::tuple<std::string, Palette>> palette{
	{"Sunny Day", {{1.00f, 1.00f, 1.00f},{0.60f, 0.54f, 0.52f}}},
	{"Bright Night", { {0.47f, 0.58f, 0.68f},  {0.32f, 0.39f, 0.57f}}},
	{"Rainy Day", { {0.62f, 0.69f, 0.67f},  {0.50f, 0.55f, 0.50f}}},
	{"Rainy Night", {{0.24f, 0.36f, 0.54f}, {0.25f, 0.31f, 0.31f}}},
};


int main() {
	GLFWwindow* window = initWindow("Assignment 0", screenWidth, screenHeight);

	ew::Shader toonShading = ew::Shader("assets/toon.vert", "assets/toon.frag");	//our shader
	ew::Model monkeyModel = ew::Model("assets/skull.obj");					//our model

	GLuint brickTexture = ew::loadTexture("assets/Txo_dokuo.png");
	GLuint zatoonTex = ew::loadTexture("assets/ZAtoon.png");

	camera.position = glm::vec3(0.0f, 0.0f, 40.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f);	//look at center of scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f;								//vertical field of view in degrees

	monkeyTransform.scale = glm::vec3(0.5f, 0.5f, 0.5f);

	//pipeline definition
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	stbi_set_flip_vertically_on_load(true);

	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		//RENDER
		glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
		//clearing the color, clearing the depth buffer (how far something is)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cameraController.move(window, &camera, deltaTime);

		////bind brick texture to texture unit 0
		glActiveTexture(GL_TEXTURE0);
		////bind my texture to the currently active texture
		glBindTexture(GL_TEXTURE_2D, brickTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, zatoonTex);

		toonShading.use();	//hides some shader code in here
		toonShading.setInt("_MainTexture", 0);
		toonShading.setInt("zatoon", 1);
		toonShading.setVec3("_EyePos", camera.position);

		//added for material struct:
		toonShading.setFloat("_Material.Ka", material.Ka);
		toonShading.setFloat("_Material.Kd", material.Kd);
		toonShading.setFloat("_Material.Ks", material.Ks);
		toonShading.setFloat("_Material.Shininess", material.Shininess);

		toonShading.setVec3("_Palette.shadow", std::get<Palette>(palette[palette_index]).shadow);
		toonShading.setVec3("_Palette.highlight", std::get<Palette>(palette[palette_index]).highlight);

		toonShading.setMat4("_Model", glm::mat4(1.0f));
		toonShading.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());

		stbi_set_flip_vertically_on_load(true);

		monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));

		//combine translation, rotation, and scale into 4x4 matrix
		toonShading.setMat4("_Model", monkeyTransform.modelMatrix());

		monkeyModel.draw();	//draw monkey model using current shader

		//draw UI
		drawUI();

		//swap buffers (swap wahatever on screen, and whatever we write to)
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

	if (ImGui::BeginCombo("Palette", std::get<std::string>(palette[palette_index]).c_str()))
	{
		for (auto n = 0; n < palette.size(); ++n)
		{
			auto is_selected = (std::get<0>(palette[palette_index]) == std::get<0>(palette[n]));
			if (ImGui::Selectable(std::get<std::string>(palette[n]).c_str(), is_selected))
			{
				palette_index = n;
			}
			if (is_selected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::ColorEdit3("Highlight", &std::get<Palette>(palette[palette_index]).highlight[0]);
	ImGui::ColorEdit3("Shadow", &std::get<Palette>(palette[palette_index]).shadow[0]);

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

