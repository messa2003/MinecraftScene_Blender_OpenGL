//
//  main.cpp
//  OpenGL Advances Lighting
//
//  Created by CGIS on 28/11/16.
//  Copyright © 2016 CGIS. All rights reserved.
//

#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"

#include <iostream>
#include <random>
#include <vector>

struct RainParticle {
	glm::vec3 position;
	glm::vec3 velocity;
};

std::vector<RainParticle> rainParticles;
GLuint rainVBO, rainVAO;
gps::Shader rainShader;
bool isRaining = false;
glm::vec3 windDirection = glm::vec3(1.0f, 0.0f, 0.0f);
float windStrength = 0.0f;
float lastFrameTime = 0.0f;

int glWindowWidth = 800;
int glWindowHeight = 600;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat4 lightRotation;

glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;

gps::Camera myCamera(
				glm::vec3(0.0f, 2.0f, 5.5f), 
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.01f;

bool pressedKeys[1024];
float angleY = 0.0f;
GLfloat lightAngle;

gps::Model3D ground;
gps::Model3D lightCube;
gps::Model3D screenQuad;
gps::Model3D creeper;

gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;

bool showDepthMap;

float lastX = glWindowWidth / 2.0f;
float lastY = glWindowHeight / 2.0f;
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;

bool isFullscreen = false;
int savedWindowWidth = 0;
int savedWindowHeight = 0;
int savedWindowPosX = 0;
int savedWindowPosY = 0;

float lightCubeRotation = 0.0f;  // For rotating the light cube around its axis

glm::vec3 fogColor = glm::vec3(0.5f, 0.5f, 0.5f);
float fogDensity = 0.1f;
int fogType = 0; // 0 = linear, 1 = exponential, 2 = exponential squared

float lastScrollY = 0.0f;

enum RenderMode {
    SOLID = 0,
    WIREFRAME = 1,
    POLYGONAL = 2,
    SMOOTH = 3
};
RenderMode currentRenderMode = SOLID;

gps::SkyBox skybox;
gps::Shader skyboxShader;

gps::SkyBox skybox1;  // Blue skybox
gps::SkyBox skybox2;  // Midnight skybox
bool useFirstSkybox = true;  // Toggle between skyboxes

glm::vec3 dayLightColor = glm::vec3(1.0f, 1.0f, 0.9f);      // Warm daylight
glm::vec3 nightLightColor = glm::vec3(0.15f, 0.15f, 0.2f);  // Dim blue moonlight

glm::vec3 creeperPosition = glm::vec3(0.1f, -5.3f, -0.6f);  // Base position
float creeperFloatOffset = 0.0f;  // For floating animation
float floatAmplitude = 0.09f;     // How high it floats
float floatSpeed = 2.0f;          // Speed of floating motion

bool presentationMode = false;
float presentationTime = 0.0f;
const float PRESENTATION_DURATION = 20.0f;  // Total time for one loop in seconds

float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct CameraPoint {
    glm::vec3 position;
    glm::vec3 lookAt;
    float timeStamp;  // When to reach this point (0.0 to 1.0)
};

std::vector<CameraPoint> cameraPath = {
    // Position                     Look at point                  Time
    {{-10.0f, 5.0f, -10.0f},      {0.0f, 0.0f, 0.0f},           0.0f},    // Start point
    {{10.0f, 3.0f, -8.0f},        {0.0f, -2.0f, 0.0f},          0.25f},   // Right side view
    {{0.0f, 8.0f, 0.0f},          {0.0f, -1.0f, 0.0f},          0.5f},    // Top view
    {{-5.0f, 2.0f, 8.0f},         {0.0f, -2.0f, 0.0f},          0.75f},   // Back view
    {{-10.0f, 5.0f, -10.0f},      {0.0f, 0.0f, 0.0f},           1.0f}     // Back to start
};

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

GLenum glCheckError_(const char *file, int line) {
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

void printCameraPosition() {
	glm::vec3 pos = myCamera.getCameraPosition();
	std::cout << "Camera Position: X: " << pos.x << " Y: " << pos.y << " Z: " << pos.z << std::endl;
}

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized to width: %d, height: %d\n", width, height);
	
	// Get framebuffer size (for retina displays)
	glfwGetFramebufferSize(window, &retina_width, &retina_height);
	
	// Update viewport
	glViewport(0, 0, retina_width, retina_height);
	
	// Update projection matrix
	projection = glm::perspective(
		glm::radians(static_cast<float>(myCamera.getFOV())), 
		static_cast<float>(retina_width) / static_cast<float>(retina_height), 
		0.1f, 
		1000.0f
	);
	
	// Update projection matrix in shaders
	myCustomShader.useShaderProgram();
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
	
	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 
					  1, GL_FALSE, glm::value_ptr(projection));
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		showDepthMap = !showDepthMap;

	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		isFullscreen = !isFullscreen;
		
		if (isFullscreen) {
			// Save current window position and size
			glfwGetWindowPos(window, &savedWindowPosX, &savedWindowPosY);
			glfwGetWindowSize(window, &savedWindowWidth, &savedWindowHeight);
			
			// Get primary monitor
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			
			// Switch to fullscreen
			glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		} else {
			// Restore windowed mode
			glfwSetWindowMonitor(window, nullptr, 
							   savedWindowPosX, savedWindowPosY, 
							   savedWindowWidth, savedWindowHeight, 
							   0);
		}
	}

	if (key == GLFW_KEY_COMMA) {  // < key
		if (action == GLFW_PRESS || action == GLFW_REPEAT)
			lightCubeRotation -= 5.0f;
	}
	if (key == GLFW_KEY_PERIOD) {  // > key
		if (action == GLFW_PRESS || action == GLFW_REPEAT)
			lightCubeRotation += 5.0f;
	}

	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		isRaining = !isRaining;
		if (isRaining) {
			windStrength = 2.0f;
			std::cout << "Rain enabled" << std::endl;
		} else {
			windStrength = 0.0f;
			std::cout << "Rain disabled" << std::endl;
		}
	}

	if (key == GLFW_KEY_O && action == GLFW_PRESS) {
		useFirstSkybox = !useFirstSkybox;  // Toggle between skyboxes
		
		// Switch lighting based on skybox
		if (useFirstSkybox) {
			lightColor = dayLightColor;
			// Update light color in shader
			myCustomShader.useShaderProgram();
			glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
		} else {
			lightColor = nightLightColor;
			// Update light color in shader
			myCustomShader.useShaderProgram();
			glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
		}
	}

	if (key == GLFW_KEY_C && action == GLFW_PRESS) {
		presentationMode = !presentationMode;  // Toggle presentation mode
		if (presentationMode) {
			presentationTime = 0.0f;  // Reset time when starting
		}
	}

	// Disable normal camera movement when in presentation mode
	if (presentationMode) {
		return;
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}

	// Fog density controls
	if (key == GLFW_KEY_K && action == GLFW_PRESS) {
		fogDensity -= 0.01f;
		if (fogDensity < 0.0f) fogDensity = 0.0f;
		myCustomShader.useShaderProgram();
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity"), fogDensity);
	}
	if (key == GLFW_KEY_I && action == GLFW_PRESS) {
		fogDensity += 0.01f;
		myCustomShader.useShaderProgram();
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity"), fogDensity);
	}

	// Fog type switching
	if (key == GLFW_KEY_T && action == GLFW_PRESS) {
		fogType = (fogType + 1) % 3;  // Cycle through fog types
		myCustomShader.useShaderProgram();
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "fogType"), fogType);
	}

	// Handle render mode switches
	if (action == GLFW_PRESS) {
		switch (key) {
			case GLFW_KEY_1:
				currentRenderMode = SOLID;
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				break;
			case GLFW_KEY_2:
				currentRenderMode = WIREFRAME;
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				break;
			case GLFW_KEY_3:
				currentRenderMode = POLYGONAL;
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				break;
			case GLFW_KEY_4:
				currentRenderMode = SMOOTH;
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				break;
		}
	}

	if (key == GLFW_KEY_9 && action == GLFW_PRESS) {
		printCameraPosition();
	}
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
	lastX = xpos;
	lastY = ypos;

	const float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	// Constrain pitch to avoid camera flipping
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	myCamera.rotate(pitch, yaw);
}

void processMovement()
{
	if (pressedKeys[GLFW_KEY_Q]) {
		angleY -= 1.0f;		
	}

	if (pressedKeys[GLFW_KEY_E]) {
		angleY += 1.0f;		
	}

	if (pressedKeys[GLFW_KEY_J]) {
		lightAngle -= 1.0f;		
	}

	if (pressedKeys[GLFW_KEY_L]) {
		lightAngle += 1.0f;
	}

	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);		
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);		
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);		
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);		
	}

	// Print camera position

}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    
    //window scaling for HiDPI displays
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    //for sRBG framebuffer
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    //for antialising
    glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwMakeContextCurrent(glWindow);

	glfwSwapInterval(1);

#if not defined (__APPLE__)
    // start GLEW extension handler
    glewExperimental = GL_TRUE;
    glewInit();
#endif

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	glfwSetScrollCallback(glWindow, scrollCallback);

	return true;
}

void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise

	glEnable(GL_FRAMEBUFFER_SRGB);
}

void initObjects() {
	creeper.LoadModel("objects/creeper/creeper.obj");
	ground.LoadModel("objects/ground/scena2_test.obj");
	lightCube.LoadModel("objects/cube/cube.obj");
	screenQuad.LoadModel("objects/quad/quad.obj");
}

void initShaders() {
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	lightShader.useShaderProgram();
	
	screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
	screenQuadShader.useShaderProgram();
	
	depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
	depthMapShader.useShaderProgram();
	
	rainShader.loadShader("shaders/snow.vert", "shaders/snow.frag");
	rainShader.useShaderProgram();
	
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
}

void initUniforms() {
	myCustomShader.useShaderProgram();

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	
	projection = glm::perspective(
		glm::radians(static_cast<float>(myCamera.getFOV())), 
		static_cast<float>(retina_width) / static_cast<float>(retina_height), 
		0.1f, 
		1000.0f
	);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");	
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	//set light color
	lightColor = dayLightColor;  // Start with daylight
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	// Initialize fog uniforms
	myCustomShader.useShaderProgram();
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "fogColor"), 1, glm::value_ptr(fogColor));
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity"), fogDensity);
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "fogType"), fogType);
}

void initFBO() {
	// Create the FBO for shadow mapping
	glGenFramebuffers(1, &shadowMapFBO);
	
	// Create depth texture
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
				 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	
	// Attach depth texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	
	// Check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer not complete!" << std::endl;
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	// Further reduce the bounds of the orthographic projection
	const GLfloat near_plane = 0.1f, far_plane = 5.5f;  // Reduced far plane more
	glm::mat4 lightProjection = glm::ortho(-3.5f, 3.5f,  // Reduced width more
										  -3.5f, 3.5f,  // Reduced height more
										  near_plane, far_plane);
	
	// Apply both rotations to get the final light direction
	glm::mat4 yRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 rotatedLightDir = glm::vec3(yRotation * glm::vec4(lightDir, 0.0f));
	glm::mat4 additionalRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightCubeRotation), rotatedLightDir);
	glm::vec3 finalLightDir = glm::vec3(additionalRotation * yRotation * glm::vec4(lightDir, 0.0f));
	
	// Move light position even closer
	glm::mat4 lightView = glm::lookAt(finalLightDir * 3.0f,  // Reduced from 4.0f
									 glm::vec3(0.0f),
									 glm::vec3(0.0f, 1.0f, 0.0f));
	
	return lightProjection * lightView;
}

void drawObjects(gps::Shader shader, bool depthPass) {
	shader.useShaderProgram();
	
	// Position creeper with just floating effect
	model = glm::mat4(1.0f);
	
	// Move to creeper's position with float offset
	model = glm::translate(model, glm::vec3(creeperPosition.x,
										   creeperPosition.y + creeperFloatOffset,
										   creeperPosition.z));
	
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	creeper.Draw(shader);

	// Ground (keeping your original code)
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.5f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	ground.Draw(shader);
}

void renderRain() {
	if (!isRaining) return;

	float currentFrame = glfwGetTime();
	float deltaTime = currentFrame - lastFrameTime;
	lastFrameTime = currentFrame;

	// Update particle positions with more complex movement
	for (auto& particle : rainParticles) {
		// Basic downward movement
		particle.position.y -= 2.0f * deltaTime;
		
		// Add swirling motion
		float time = currentFrame * 0.5f;
		float swirl = sin(time + particle.position.y * 0.1f) * 0.3f;
		particle.position.x += sin(time * 0.5f + particle.position.y * 0.05f) * deltaTime * 0.5f;
		particle.position.z += cos(time * 0.5f + particle.position.x * 0.05f) * deltaTime * 0.5f;
		

		// Reset if below ground
		if (particle.position.y < -1.0f) {
			float randX = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 4.0f;
			float randZ = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 4.0f;
			float randY = ((float)rand() / RAND_MAX * 2.0f) * 2.0f; // Random height offset
			particle.position = glm::vec3(-0.193871f + randX, 8.0f + randY, 0.160195f + randZ);
		}
	}

	// Update VBO with new positions
	glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
	glBufferData(GL_ARRAY_BUFFER, rainParticles.size() * sizeof(RainParticle), 
				rainParticles.data(), GL_DYNAMIC_DRAW);

	rainShader.useShaderProgram();

	// Update uniforms
	glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "view"),
		1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "projection"),
		1, GL_FALSE, glm::value_ptr(projection));

	// Enable blending and disable depth writing (but keep depth testing)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);  // Disable depth writing
	
	// Draw particles
	glBindVertexArray(rainVAO);
	glDrawArrays(GL_POINTS, 0, rainParticles.size());
	glBindVertexArray(0);

	// Restore depth writing
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

void initSkybox() {
	// First skybox (blue)
	std::vector<const GLchar*> faces1;
	faces1.push_back("skybox/blue_lf.tga");  // Left   (-X)
	faces1.push_back("skybox/blue_rt.tga");  // Right  (+X)
	faces1.push_back("skybox/blue_up.tga");  // Up     (+Y)
	faces1.push_back("skybox/blue_dn.tga");  // Down   (-Y)
	faces1.push_back("skybox/blue_ft.tga");  // Front  (+Z)
	faces1.push_back("skybox/blue_bk.tga");  // Back   (-Z)
	skybox1.Load(faces1);

	// Second skybox (midnight)
	std::vector<const GLchar*> faces2;
	faces2.push_back("skybox/midnight-silence_lf.tga");
	faces2.push_back("skybox/midnight-silence_rt.tga");
	faces2.push_back("skybox/midnight-silence_up.tga");
	faces2.push_back("skybox/midnight-silence_dn.tga");
	faces2.push_back("skybox/midnight-silence_ft.tga");
	faces2.push_back("skybox/midnight-silence_bk.tga");
	skybox2.Load(faces2);
}

void renderScene() {
	// depth maps creation pass
	depthMapShader.useShaderProgram();
	
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceMatrix"),
					1,
					GL_FALSE,
					glm::value_ptr(computeLightSpaceTrMatrix()));
	
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	
	drawObjects(depthMapShader, true);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	// final scene rendering pass
	glViewport(0, 0, retina_width, retina_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw appropriate skybox
	view = glm::mat4(glm::mat3(myCamera.getViewMatrix())); // Remove translation
	if (useFirstSkybox) {
		skybox1.Draw(skyboxShader, view, projection);
	} else {
		skybox2.Draw(skyboxShader, view, projection);
	}
	
	// Reset view matrix for other objects
	view = myCamera.getViewMatrix();

	// render depth map on screen - toggled with the M key
	if (showDepthMap) {
		glClear(GL_COLOR_BUFFER_BIT);

		screenQuadShader.useShaderProgram();

		//bind the depth map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

		glDisable(GL_DEPTH_TEST);
		screenQuad.Draw(screenQuadShader);
		glEnable(GL_DEPTH_TEST);
	}
	else {
		// final scene rendering pass (with shadows)
		myCustomShader.useShaderProgram();
		
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		
		// First rotate around Y axis (lightAngle)
		lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		// Then rotate around the light direction (lightCubeRotation)
		glm::vec3 rotatedLightDir = glm::vec3(lightRotation * glm::vec4(lightDir, 0.0f));
		glm::mat4 additionalRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightCubeRotation), rotatedLightDir);
		lightRotation = additionalRotation * lightRotation;
		
		// Update light direction with both rotations
		glm::vec3 finalLightDir = glm::vec3(lightRotation * glm::vec4(lightDir, 0.0f));
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * finalLightDir));
		
		//bind the shadow map
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);
		
		glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
						1,
						GL_FALSE,
						glm::value_ptr(computeLightSpaceTrMatrix()));
		
		// Set render mode uniform
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "renderMode"), currentRenderMode);

		drawObjects(myCustomShader, false);
		
		//draw a white cube at the light position
		lightShader.useShaderProgram();
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
		
		// Use the same rotation for the light cube
		model = lightRotation;
		model = glm::translate(model, lightDir * 5.0f);
		model = glm::scale(model, glm::vec3(0.3f, 0.3f, 0.3f));
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
		
		lightCube.Draw(lightShader);
	}

	if (isRaining) {
		renderRain();  // Only render if rain is enabled
	}
}

void cleanup() {
	glDeleteTextures(1,& depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	glfwDestroyWindow(glWindow);
	//close GL context and any other GLFW resources
	glfwTerminate();

	glDeleteVertexArrays(1, &rainVAO);
	glDeleteBuffers(1, &rainVBO);
}

void initSnow() {
	const int NUM_PARTICLES = 20000;
	const float AREA_SIZE = 4.0f;      
	const float MAX_HEIGHT = 15.0f;    // Increased height
	
	// Fixed snow center at exact coordinates, moved higher up
	glm::vec3 snowCenter = glm::vec3(-0.193871f, 8.0f, 0.160195f);  // Increased Y coordinate
	
	// Enable point sprite and set point size parameters
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_POINT_SPRITE);
	
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> xzDist(-AREA_SIZE/2, AREA_SIZE/2);
	std::uniform_real_distribution<float> yDist(0.0f, MAX_HEIGHT);
	std::uniform_real_distribution<float> velocityVariation(0.6f, 1.2f);
	
	rainParticles.resize(NUM_PARTICLES);
	
	for(auto& particle : rainParticles) {
		particle.position = snowCenter + glm::vec3(
			xzDist(gen),
			yDist(gen),
			xzDist(gen)
		);
		particle.velocity = glm::vec3(
			0.0f, 
			-5.0f * velocityVariation(gen),
			0.0f
		);
	}
	
	// Create and set up VAO/VBO for rain particles
	glGenVertexArrays(1, &rainVAO);
	glGenBuffers(1, &rainVBO);
	
	glBindVertexArray(rainVAO);
	glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
	glBufferData(GL_ARRAY_BUFFER, rainParticles.size() * sizeof(RainParticle), 
				rainParticles.data(), GL_DYNAMIC_DRAW);
	
	// Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RainParticle), 
						 (void*)offsetof(RainParticle, position));
	
	// Velocity attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RainParticle), 
						 (void*)offsetof(RainParticle, velocity));
	
	glBindVertexArray(0);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	myCamera.zoom(yoffset);
	
	// Update projection matrix with new FOV
	projection = glm::perspective(
		glm::radians(static_cast<float>(myCamera.getFOV())), 
		static_cast<float>(retina_width) / static_cast<float>(retina_height), 
		0.1f, 
		1000.0f
	);
	
	// Update projection matrix in shaders
	myCustomShader.useShaderProgram();
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
	
	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 
					  1, GL_FALSE, glm::value_ptr(projection));
}

void updateCreeper() {
	// Only update floating motion
	float time = glfwGetTime();
	creeperFloatOffset = sin(time * floatSpeed) * floatAmplitude;
}

void updateCameraPresentation() {
	if (!presentationMode) return;
	
	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	
	// Update presentation time
	presentationTime += deltaTime;
	if (presentationTime >= PRESENTATION_DURATION) {
		presentationTime = 0.0f;
	}
	
	// Calculate progress (0.0 to 1.0)
	float progress = presentationTime / PRESENTATION_DURATION;
	
	// Find the current and next camera points
	CameraPoint current, next;
	for (size_t i = 0; i < cameraPath.size() - 1; i++) {
		if (progress >= cameraPath[i].timeStamp && progress < cameraPath[i + 1].timeStamp) {
			current = cameraPath[i];
			next = cameraPath[i + 1];
			
			// Calculate local progress between these two points
			float localProgress = (progress - current.timeStamp) / 
								(next.timeStamp - current.timeStamp);
			
			// Interpolate position and look-at point
			glm::vec3 newPosition = glm::mix(current.position, next.position, localProgress);
			glm::vec3 newLookAt = glm::mix(current.lookAt, next.lookAt, localProgress);
			
			// Update camera
			myCamera.setPosition(newPosition);
			myCamera.setTarget(newLookAt);
			break;
		}
	}
}

int main(int argc, const char * argv[]) {

	if (!initOpenGLWindow()) {
		glfwTerminate();
		return 1;
	}

	initOpenGLState();
	initObjects();
	initShaders();
	initUniforms();
	initFBO();

	glCheckError();

	initSnow();
	initSkybox();

	while (!glfwWindowShouldClose(glWindow)) {
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		
		if (!presentationMode) {
			processMovement();
		}
		updateCameraPresentation();
		updateCreeper();
		printCameraPosition();
		renderScene();
		
		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	cleanup();

	return 0;
}
