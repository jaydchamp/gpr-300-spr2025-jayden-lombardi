#include <stdio.h>
#include <math.h>
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
void renderToGBuff(ew::Shader& gBuffShader, GLuint texture, ew::Model model);
void drawScene(ew::Camera& camera, ew::Shader& shader, ew::Model& model, ew::Mesh& planeMesh);
void renderShadowMap(ew::Shader& depthShader, ew::Model& model, ew::Mesh& planeMesh);
void lightingPass(ew::Shader& deffShader);
void resetCamera(ew::Camera* camera, ew::CameraController* controller);
void initCamera();
glm::mat4 calculateLightSpaceMatrix();
void definePipline();
void drawUI();
void checkShaderCompilation(GLuint shader, const char* type);
#define glCheckError() glCheckError_(__FILE__, __LINE__)

//Global state
const int SHADOW_WIDTH = 2048;
const int SHADOW_HEIGHT = 2048;
const int GRID_SIZE = 8;
const float SPACING = 3.0f;
const float START_POS = -((GRID_SIZE - 1) * SPACING) / 2.0f; 
const float GRID_OFFSET = SPACING * (GRID_SIZE / 2.0f);
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

ew::Camera camera;	//our camera
ew::CameraController cameraController;
std::vector<ew::Transform> monkeyTransform;
ew::Transform lightSphereTransform;
ew::Mesh sphere; //light sphere
ew::Mesh plane; //ground plane
ew::Transform planeTransform; //ground plane

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
	float ambientK = 0.4;
	float diffuseK = 0.5;
	float specularK = 0.5;
	float shininess = 128;
}materials;

const int MAX_POINT_LIGHTS = 64;
struct PointLight
{
	glm::vec3 pos;
	float radius;
	glm::vec4 color;
}pointLights[MAX_POINT_LIGHTS]; 
int currentPointLightCount = 4;

struct DirectionalLight 
{
	glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);
	glm::vec3 color = glm::vec3(1.0f);
	float intensity = 1.0f;

	// Shadow mapping parameters
	float minBias = 0.001f;
	float maxBias = 0.01f;
	float softness = 0.0f;
} directionalLight;

struct FrameBuffer
{
	GLuint fbo;

	GLuint colorBuffers[3];

	GLuint color;
	GLuint position;
	GLuint normal;
	GLuint material;
	GLuint depthTexture;

	unsigned int width;
	unsigned int height;

	void Initialize(unsigned int iWidth, unsigned int iHeight)
	{
		//bind buffer
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		{
			width = iWidth;
			height = iHeight;

			//color attachment
			glGenTextures(1, &color);
			glBindTexture(GL_TEXTURE_2D, color);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, iWidth, iHeight, 0, GL_RGB, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

			//position attachment
			glGenTextures(1, &position);
			glBindTexture(GL_TEXTURE_2D, position);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, iWidth, iHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, position, 0);

			//normal attachment
			glGenTextures(1, &normal);
			glBindTexture(GL_TEXTURE_2D, normal);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, iWidth, iHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, normal, 0);

			//normal attachment
			glGenTextures(1, &material);
			glBindTexture(GL_TEXTURE_2D, material);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, iWidth, iHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, material, 0);

			const GLenum arr[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers(4, arr);

			//depth attachment
			glGenTextures(1, &depthTexture);
			glBindTexture(GL_TEXTURE_2D, depthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, iWidth, iHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

			GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
			{
				printf("Framebuffer incomplete: %d", fboStatus);
			}
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}frameBuffer;

FrameBuffer createGBuffer(unsigned int width, unsigned int height) 
{
	FrameBuffer framebuffer;
	framebuffer.width = width;
	framebuffer.height = height;

	glCreateFramebuffers(1, &framebuffer.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);

	int formats[3] = 
	{
		GL_RGB32F, //0 = World Position 
		GL_RGB16F, //1 = World Normal
		GL_RGB16F  //2 = Albedo
	};

	//Create 3 color textures
	for (size_t i = 0; i < 3; i++)
	{
		glGenTextures(1, &framebuffer.colorBuffers[i]);
		glBindTexture(GL_TEXTURE_2D, framebuffer.colorBuffers[i]);
		glTexStorage2D(GL_TEXTURE_2D, 1, formats[i], width, height);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, framebuffer.colorBuffers[i], 0);
	}
	const GLenum drawBuffers[3] = 
	{
			GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
	};
	glDrawBuffers(3, drawBuffers);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return framebuffer;
}

FrameBuffer gBuffer;

struct ShadowMap 
{
	GLuint fbo;
	GLuint depthTexture;

	void Initalize() {
		glGenFramebuffers(1, &fbo);
		glGenTextures(1, &depthTexture);

		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT,
			0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			printf("ERROR: Framebuffer is not complete!\n");
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
} shadowMap;

int main() {
	GLFWwindow* window = initWindow("Assignment4", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	ew::Shader depthShader = ew::Shader("D:/Users/Jayden/Desktop/MONT_REAL/Animations/gpr-300-spr2025-jayden-lombardi/assignments/assignment3/assets/depth.vert", "D:/Users/Jayden/Desktop/MONT_REAL/Animations/gpr-300-spr2025-jayden-lombardi/assignments/assignment3/assets/depth.frag"); 
	ew::Shader gBuffShader = ew::Shader("D:/Users/Jayden/Desktop/MONT_REAL/Animations/gpr-300-spr2025-jayden-lombardi/assignments/assignment3/assets/geoPass.vert", "D:/Users/Jayden/Desktop/MONT_REAL/Animations/gpr-300-spr2025-jayden-lombardi/assignments/assignment3/assets/geoPass.frag");
	ew::Shader deferredShader = ew::Shader("D:/Users/Jayden/Desktop/MONT_REAL/Animations/gpr-300-spr2025-jayden-lombardi/assignments/assignment3/assets/deferredLit.vert", "D:/Users/Jayden/Desktop/MONT_REAL/Animations/gpr-300-spr2025-jayden-lombardi/assignments/assignment3/assets/deferredLit.frag");
	//ew::Shader depthShader = ew::Shader("newassets/depth.vert", "newassets/depth.frag");
	//ew::Shader gBuffShader = ew::Shader("newassets/geoPass.vert", "newassets/geoPass.frag");
	//ew::Shader deferredShader = ew::Shader("newassets/deferredLit.vert", "newassets/deferredLit.frag");

	ew::Model monkeyModel = ew::Model("assets/suzanne.obj");		
	GLuint brickTexture = ew::loadTexture("assets/brick_color.jpg");

	initCamera();

	//enable depth test and culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	const int GRID_SIZE = 8;
	const float SPACING = 3.0f;
	const float START_POS = -((GRID_SIZE - 1) * SPACING) / 2.0f;

	monkeyTransform.clear();

	for (int x = 0; x < GRID_SIZE; x++) {
		for (int z = 0; z < GRID_SIZE; z++) {
			ew::Transform transform;
			transform.position = glm::vec3(
				START_POS + x * SPACING,
				1.0f,
				START_POS + z * SPACING
			);
			transform.scale = glm::vec3(0.7f);
			monkeyTransform.push_back(transform);
		}
	}

	shadowMap.Initalize();
	gBuffer = createGBuffer(screenWidth, screenHeight);
	frameBuffer.Initialize(screenWidth, screenHeight);

	sphere.load(ew::createSphere(0.5f, 4));
	plane.load(ew::createPlane(50.0f, 50.0f, 100));

	fullscreen_quad.Initalize();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		cameraController.move(window, &camera, deltaTime);

		renderToGBuff(gBuffShader, brickTexture, monkeyModel);

		drawScene(camera, gBuffShader, monkeyModel, plane);

		renderShadowMap(depthShader, monkeyModel, plane);

		lightingPass(deferredShader);

		//post_process(geoShader);

		drawUI();
		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}

void renderToGBuff(ew::Shader& gBuffShader, GLuint texture, ew::Model model)
{
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.fbo);
	glViewport(0, 0, gBuffer.width, gBuffer.height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	gBuffShader.use();
	gBuffShader.setInt("MainTex", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
}

void drawScene(ew::Camera& camera, ew::Shader& shader, ew::Model& model, ew::Mesh& planeMesh)
{
	shader.use();

	//
	shader.setMat4("ViewProj", camera.projectionMatrix() * camera.viewMatrix());

	// Render plane
	shader.setMat4("Model", planeTransform.modelMatrix());
	planeMesh.draw();

	// Render all monkeys
	for (size_t i = 0; i < monkeyTransform.size(); i++)
	{
		shader.setMat4("Model", monkeyTransform[i].modelMatrix());
		model.draw();
	}
}

void renderShadowMap(ew::Shader& depthShader, ew::Model& model, ew::Mesh& planeMesh) 
{
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT); 
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.fbo); 
	glClear(GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_BACK); 

	glm::mat4 lightSpaceMatrix = calculateLightSpaceMatrix(); 

	depthShader.use(); 
	depthShader.setMat4("LightSpaceMatrix", lightSpaceMatrix);

	// Render all 64 monkeys
	for (size_t i = 0; i < monkeyTransform.size(); i++) { 
		depthShader.setMat4("Model", monkeyTransform[i].modelMatrix()); 
		model.draw(); 
	}

	// Render plane
	depthShader.setMat4("Model", planeTransform.modelMatrix()); 
	planeMesh.draw(); 

	glBindFramebuffer(GL_FRAMEBUFFER, 0); 
}

void lightingPass(ew::Shader& deffShader)
{
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.fbo);
	glViewport(0, 0, frameBuffer.width, frameBuffer.height);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	deffShader.use();

	// Set lighting uniforms
	deffShader.setVec3("LightDir", glm::normalize(directionalLight.direction));
	deffShader.setVec3("LightColor", directionalLight.color);
	deffShader.setFloat("LightIntensity", directionalLight.intensity);

	// Set Camera Uniforms
	deffShader.setVec3("CameraPos", camera.position);

	// Set material properties
	deffShader.setFloat("material.ambientK", materials.ambientK);
	deffShader.setFloat("material.diffuseK", materials.diffuseK);
	deffShader.setFloat("material.specularK", materials.specularK);
	deffShader.setFloat("material.shininess", materials.shininess);

	// Set shadow mapping parameters
	deffShader.setMat4("LightSpaceMatrix", calculateLightSpaceMatrix());
	deffShader.setFloat("MinBias", directionalLight.minBias);
	deffShader.setFloat("MaxBias", directionalLight.maxBias);
	deffShader.setFloat("ShadowSof", directionalLight.softness);

	deffShader.setInt("PointLightCount", currentPointLightCount);

	for (int i = 0; i < currentPointLightCount; i++) {
		// Spread lights around the grid based on current light count
		switch (i % 4) {
		case 0:
			pointLights[i].pos = glm::vec3(-GRID_OFFSET, 5.0f, -GRID_OFFSET);
			pointLights[i].color = glm::vec4(1.0f, 0.3f, 0.3f, 2.0f);  // Red
			break;
		case 1:
			pointLights[i].pos = glm::vec3(GRID_OFFSET, 5.0f, -GRID_OFFSET);
			pointLights[i].color = glm::vec4(0.3f, 1.0f, 0.3f, 2.0f);  // Green
			break;
		case 2:
			pointLights[i].pos = glm::vec3(-GRID_OFFSET, 5.0f, GRID_OFFSET);
			pointLights[i].color = glm::vec4(0.3f, 0.3f, 1.0f, 2.0f);  // Blue
			break;
		case 3:
			pointLights[i].pos = glm::vec3(GRID_OFFSET, 5.0f, GRID_OFFSET);
			pointLights[i].color = glm::vec4(1.0f, 1.0f, 0.3f, 2.0f);  // Yellow
			break;
		}

		pointLights[i].radius = 15.0f;

		std::string prefix = "PointLights[" + std::to_string(i) + "].";
		deffShader.setVec3(prefix + "pos", pointLights[i].pos);
		deffShader.setFloat(prefix + "radius", pointLights[i].radius);
		deffShader.setVec4(prefix + "color", pointLights[i].color);
	}

	// Bind G-Buffer textures
	deffShader.setInt("Positions", 0);
	deffShader.setInt("Normals", 1);
	deffShader.setInt("Albedo", 2);
	deffShader.setInt("ShadowMap", 3);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBuffer.colorBuffers[0]);  // Position
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gBuffer.colorBuffers[1]);  // Normal
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gBuffer.colorBuffers[2]);  // Albedo
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, shadowMap.depthTexture);   // Shadow map

	// Draw fullscreen triangle
	glBindVertexArray(fullscreen_quad.vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// 4. Copy the result to the default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screenWidth, screenHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Use a simple shader to render the final image to the screen
	// For simplicity, we're using the default screen rendering
	glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBuffer.fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, frameBuffer.width, frameBuffer.height, 0, 0, screenWidth, screenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
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

glm::mat4 calculateLightSpaceMatrix() {
	float near = 1.0f;
	float far = 15.0f;
	float size = 5.0f;

	glm::mat4 lightProjection = glm::perspective(glm::radians(45.0f), 1.0f, near, far);

	glm::mat4 lightView = glm::lookAt(
		-directionalLight.direction * 5.0f,
		glm::vec3(0.0f),
		glm::vec3(1.0f, 1.0f, 0.0f)
	);

	return lightProjection * lightView;
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
	ImGui::Separator();
	if (ImGui::CollapsingHeader("Material")) 
	{
		ImGui::SliderFloat("Ambient K", &materials.ambientK, 0.0f, 1.0f);
		ImGui::SliderFloat("Diffuse K", &materials.diffuseK, 0.0f, 1.0f);
		ImGui::SliderFloat("Specular K", &materials.specularK, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &materials.shininess, 1.0f, 256.0f);
	}
	ImGui::Separator();
	if (ImGui::CollapsingHeader("Light")) 
	{
		// Directional Light Color
		ImGui::ColorEdit3("Direction Color", &directionalLight.color.x);

		// Directional Light Direction
		ImGui::SliderFloat("Dir X", &directionalLight.direction.x, -1.0f, 1.0f);
		ImGui::SliderFloat("Dir Y", &directionalLight.direction.y, -1.0f, 1.0f);
		ImGui::SliderFloat("Dir Z", &directionalLight.direction.z, -1.0f, 1.0f);

		// Point Light Controls
		ImGui::SliderFloat("Point Light Radius", &pointLights[0].radius, 1.0f, 20.0f, "%.3f");
		for (int i = 1; i < MAX_POINT_LIGHTS; i++) 
		{
			pointLights[i].radius = pointLights[0].radius;
		}

		ImGui::SliderFloat("Point Light Intensity", &pointLights[0].color.w, 0.0f, 5.0f, "%.3f");
		for (int i = 1; i < MAX_POINT_LIGHTS; i++) 
		{
			pointLights[i].color.w = pointLights[0].color.w;
		}

		ImGui::SliderInt("Num Lights", &currentPointLightCount, 1, MAX_POINT_LIGHTS);
	}
	ImGui::Separator();
	if (ImGui::CollapsingHeader("Shadows")) 
	{
		ImGui::SliderFloat("Min Bias", &directionalLight.minBias, 0.0001f, 0.01f, "%.5f");
		ImGui::SliderFloat("Max Bias", &directionalLight.maxBias, 0.001f, 0.05f, "%.5f");
		ImGui::SliderFloat("Softness", &directionalLight.softness, 0.0f, 5.0f, "%.2f");
	}
	ImGui::End();

	ImGui::Begin("GBuffers");
	ImGui::Separator();
	ImGui::Image((ImTextureID)(intptr_t)frameBuffer.color, ImVec2(300, 200));
	ImGui::Separator();
	ImGui::Image((ImTextureID)(intptr_t)gBuffer.colorBuffers[0], ImVec2(300, 200));
	ImGui::Separator();
	ImGui::Image((ImTextureID)(intptr_t)gBuffer.colorBuffers[1], ImVec2(300, 200));
	ImGui::Separator();
	ImGui::Image((ImTextureID)(intptr_t)gBuffer.colorBuffers[2], ImVec2(300, 200));
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

void checkShaderCompilation(GLuint shader, const char* type) 
{
	GLint success;
	GLchar infoLog[1024];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 1024, NULL, infoLog);
		printf("ERROR::SHADER_COMPILATION_ERROR of type: %s\n%s\n", type, infoLog);
	}
}