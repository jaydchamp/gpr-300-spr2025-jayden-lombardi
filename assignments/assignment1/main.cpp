#include <stdio.h>
#include <math.h>

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

//FrameBuffer Struct
struct FrameBuffer
{
	GLuint fbo;
	GLuint color0;
	GLuint color1;
	GLuint depth;
}framebuffer;

//Function Declarations
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();

//Global Variables
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

float boxBlurStrength = 9.0f;
float gaussBlurStrength = 16.0f;
float noiseStrength = 2.0f;
float exposeHDR = 1.0f;
float vigStrength = 3.0f;
glm::vec3 radialDistort = glm::vec3(0.2, 0.2, 0.2);
glm::vec2 tanDistort = glm::vec2(0.2, 0.2);

ew::Camera camera;
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

struct FullScreenQuad
{
	GLuint vao;
	GLuint vbo;
}fullscreen_quad;

static float quad_vertices[] ={
	// pos (x, y) texcoord (u, v)
	-1.0f,  1.0f, 0.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 0.0f,
	1.0f, -1.0f, 1.0f, 0.0f,

	-1.0f,  1.0f, 0.0f, 1.0f,
	1.0f, -1.0f, 1.0f, 0.0f,
	1.0f,  1.0f, 1.0f, 1.0f,
};

static int effect_index = 0;
static std::vector<std::string> post_processing_effects = {
	"None",
	"Fullscreen",
	"Chromatic",
	"Box Blur",
	"Gaussian Blur",
	"Edge Detection",
	"Sharpen",
	"Grain",		
	"Greyscale",
	"Inverse",
	"HDR",			
	"Vignette",
	"Gamma",
	"Lens Distortion",
};

void render(ew::Shader shader, ew::Model model, GLuint texture)
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);

		//glBindTextureUnit(0, texture);

		shader.use();
		shader.setInt("_MainTexture", 0);
		shader.setVec3("_EyePos", camera.position);

		//added for material struct:
		shader.setFloat("_Material.Ka", material.Ka);
		shader.setFloat("_Material.Kd", material.Kd);
		shader.setFloat("_Material.Ks", material.Ks);
		shader.setFloat("_Material.Shininess", material.Shininess);

		shader.setMat4("_Model", glm::mat4(1.0f));
		shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		shader.setMat4("_Model", monkeyTransform.modelMatrix());
		//draw monkey model using current shader

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);

		model.draw();

		glBindTextureUnit(0, 0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void post_process(ew::Shader& shader)
{
	shader.use();
	shader.setInt("texture0", 0);

	//box blur strength
	shader.setFloat("strength", boxBlurStrength);
	//gaussian blur strength
	shader.setFloat("strength", gaussBlurStrength);
	//Grain noise strength
	shader.setFloat("noiseStrength", noiseStrength);
	//HDR exposure
	shader.setFloat("_exposure", exposeHDR);
	//vignette strength
	shader.setFloat("vigStrength", vigStrength);
	//radial distortion
	shader.setVec3("radialDistortionParams", radialDistort);
	//tangential distortion
	shader.setVec2("tangentialDistortionParams", tanDistort);

	glBindVertexArray(fullscreen_quad.vao);
	{
		glDisable(GL_DEPTH_TEST);

		glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, framebuffer.color0);
		glBindTextureUnit(0, framebuffer.color0);

		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	glBindVertexArray(0);
}

void initCam(ew::Camera& camera)
{
	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f);	//look at center of scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f;								//vertical field of view in degrees
}

void CreateFullScreenQuad()
{
	glGenVertexArrays(1, &fullscreen_quad.vao);
	glGenBuffers(1, &fullscreen_quad.vbo);

	glBindVertexArray(fullscreen_quad.vao);
	{
		glBindBuffer(GL_ARRAY_BUFFER, fullscreen_quad.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(sizeof(float) * 2));
	}
	glBindVertexArray(0);
}

void CreateFrameBuffer(FrameBuffer& framebuff, GLFWwindow* window)
{
	glGenFramebuffers(1, &framebuff.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuff.fbo);
	{
		//create color attachment texture
		glGenTextures(1, &framebuff.color0);
		glBindTexture(GL_TEXTURE_2D, framebuff.color0);
		//change from gl_rgb to 16 float (HDR)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuff.color0, 0);

		//making a brightness attachment
		glGenTextures(1, &framebuff.color1);
		glBindTexture(GL_TEXTURE_2D, framebuff.color1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, framebuff.color1, 0);

		GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
		{
			printf("Framebuffer incomplete: %d", fboStatus);
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int main() {
	//define shader, model, and texture
	GLFWwindow* window = initWindow("Assignment 1", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	GLuint brickTexture = ew::loadTexture("assets/brick_color.jpg");
	ew::Model monkeyModel = ew::Model("assets/suzanne.obj");

	//shaders
		ew::Shader litShader = ew::Shader("assets/lit.vert", "assets/lit.frag");
		ew::Shader fullscreenShader = ew::Shader("assets/fullscreen.vert", "assets/fullscreen.frag");
		ew::Shader chromaticShader = ew::Shader("assets/chromatic.vert", "assets/chromatic.frag");
		ew::Shader boxBlurShader = ew::Shader("assets/blur.vert", "assets/blur.frag");
		ew::Shader gaussBlurShader = ew::Shader("assets/blur.vert", "assets/gaussBlur.frag");
		ew::Shader edgeShader = ew::Shader("assets/blur.vert", "assets/edge.frag");
		ew::Shader sharpenShader = ew::Shader("assets/sharpen.vert", "assets/sharpen.frag");
		ew::Shader grainShader = ew::Shader("assets/grain.vert", "assets/grain.frag");
		ew::Shader grayShader = ew::Shader("assets/grey.vert", "assets/grey.frag");
		ew::Shader inverseShader = ew::Shader("assets/inverse.vert", "assets/inverse.frag");
		ew::Shader hdrShader = ew::Shader("assets/HDR.vert", "assets/HDR.frag"); 
		ew::Shader vignetteShader = ew::Shader("assets/Vignette.vert", "assets/Vignette.frag");
		ew::Shader gammaShader = ew::Shader("assets/Gamma.vert", "assets/Gamma.frag");
		ew::Shader distortShader = ew::Shader("assets/blur.vert", "assets/distort.frag");

	//init camera
	initCam(camera);

	//init fullscreen quad
	CreateFullScreenQuad();

	//init frame buffer
	CreateFrameBuffer(framebuffer, window);

	while (!glfwWindowShouldClose(window)) 
	{
		glfwPollEvents();
		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;
		monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));

		render(fullscreenShader, monkeyModel, brickTexture);
		cameraController.move(window, &camera, deltaTime);

		switch (effect_index)
		{
		case 1:
			post_process(fullscreenShader);
			break;
		case 2:
			post_process(chromaticShader);
			break;
		case 3:
			post_process(boxBlurShader); 
			break;
		case 4:
			post_process(gaussBlurShader);  
			break;
		case 5:
			post_process(edgeShader);  
			break;
		case 6:
			post_process(sharpenShader);
			break;
		case 7:
			post_process(grainShader);
			break;
		case 8:
			post_process(grayShader);
			break;
		case 9:
			post_process(inverseShader);
			break;
		case 10:
			post_process(hdrShader);
			break;
		case 11:
			post_process(vignetteShader);
			break;
		case 12:
			post_process(gammaShader);
			break;
		case 13:
			post_process(distortShader);
			break;
		default:
			post_process(litShader);
			break;
		}

		//draw UI keep
		drawUI();

		//swap buffers
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

void drawUI() 
{
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	//IMGUI debugging? not in the render loop?
	ImGui::Image((ImTextureID)(intptr_t)framebuffer.color0, ImVec2(800, 600)); 

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
	if (ImGui::BeginCombo("Effect", post_processing_effects[effect_index].c_str()))
	{
		for (auto n = 0; n < post_processing_effects.size(); ++n)
		{
			auto is_selected = (post_processing_effects[effect_index] == post_processing_effects[n]);
			if (ImGui::Selectable(post_processing_effects[n].c_str(), is_selected))
			{
				effect_index = n;
			}
			if (is_selected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	switch (effect_index)
	{	
	case 3: //box blur
		ImGui::SliderFloat("Box Blur Strength", &boxBlurStrength, 0.0f, 20.0f);
		break;
	case 4: //gauss blur
		ImGui::SliderFloat("Gaussian Blur Strength", &gaussBlurStrength, 0.0f, 30.0f);
		break;
	case 7: //grain 
		ImGui::SliderFloat("Grain Noise Strength", &noiseStrength, 0.0f, 10.0f);
		break;
	case 10: //hdr
		ImGui::SliderFloat("HDR Exposure", &exposeHDR, 0.0f, 10.0f);
		break;
	case 11: //vignette
		ImGui::SliderFloat("Vignette Strength", &vigStrength, 0.0f, 1.0f);
		break;
	case 13: //lens distortion not working??
		ImGui::SliderFloat("Rad. Distort X", &radialDistort.x, 0.0f, 1.0f);
		ImGui::SliderFloat("Rad. Distort Y", &radialDistort.y, 0.0f, 1.0f);
		ImGui::SliderFloat("Rad. Distort Z", &radialDistort.z, 0.0f, 1.0f);
		ImGui::SliderFloat("Tang. Distort X", &tanDistort.x, 0.0f, 1.0f);
		ImGui::SliderFloat("Tang. Distort Y", &tanDistort.y, 0.0f, 1.0f);
		break;
	default:
		break;
	}
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