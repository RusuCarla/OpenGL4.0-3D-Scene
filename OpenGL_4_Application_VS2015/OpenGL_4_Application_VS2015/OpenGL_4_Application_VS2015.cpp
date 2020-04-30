//
//  main.cpp
//  OpenGL Shadows
//
//  Created by CGIS on 05/12/16.
//  Copyright � 2016 CGIS. All rights reserved.
//

#define GLEW_STATIC

#include <iostream>
#include "glm/glm.hpp"//core glm functionality
#include "glm/gtc/matrix_transform.hpp"//glm extension for generating common transformation matrices
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "GLEW/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include "Shader.hpp"
#include "Camera.hpp"
#include "glm/gtx/transform.hpp"
#define TINYOBJLOADER_IMPLEMENTATION

#include "Model3D.hpp"
#include "Mesh.hpp"

int glWindowWidth = 1280;
int glWindowHeight = 960; 
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

const GLuint SHADOW_WIDTH = 8192, SHADOW_HEIGHT = 8192;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat3 lightDirMatrix;
GLuint lightDirMatrixLoc;

//directional light direction
glm::vec3 lightDir;
GLuint lightDirLoc;

//point lights positions
glm::vec3 lightPos1;
GLuint lightPos1Loc;
glm::vec3 lightPos2;
GLuint lightPos2Loc;
glm::vec3 lightPos3;
GLuint lightPos3Loc;

//directional light color
glm::vec3 lightColor;
GLuint lightColorLoc;

//alpha channel for transparent objects
float alpha;
GLuint alphaLoc;

gps::Camera myCamera(glm::vec3(0.0f, 1.0f, 2.5f), glm::vec3(0.0f, 0.0f, 0.0f));
GLfloat cameraSpeed = 0.1f;

bool pressedKeys[1024];
GLfloat angle;
GLfloat lightAngle;

//models
gps::Model3D myModel;
gps::Model3D boat;
gps::Model3D wheel;
gps::Model3D crab;
gps::Model3D car;
gps::Model3D lake;
gps::Model3D cloud;
gps::Model3D lightCube;
gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader depthMapShader;
GLuint shadowMapFBO;
GLuint depthMapTexture;

//tour animation variables
bool ok = false;
bool inTour = false;
int i = 0;

bool night = false;

//crab animation variables
float crab_angle, crab_posy, crab_rotate = 0;
float crab_speed = 0.005;
float ca = 0.8f;
float cy = 0.01f;
bool ok1 = true;
bool ok2 = false;

//boat animation variables
float boat_angle, boat_rotate = 0;
float boat_posx = -0.05f;
float boat_posy = -2.3f;
float boat_posz = 11.73f;
float boat_speed = 0.04;

//wheel animation variables
float wheel_angle = 0;

//cloud animatoin variables
float cloud_posx, cloud_posy, cloud_posz = 0;
float cloud_speed = 0.05;
int count, k = 0;
bool debouncer = true;
bool started = false;


GLenum glCheckError_(const char *file, int line)
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
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height)
{
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	//TODO
	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	myCustomShader.useShaderProgram();

	//set projection matrix
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	//send matrix data to shader
	GLint projLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	
	lightShader.useShaderProgram();
	
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//set Viewport transform
	glViewport(0, 0, retina_width, retina_height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	myCamera.mouse_callback(xpos,ypos);
}

bool collision_detection(glm::vec3 camera, gps::Model3D obj)
{
	return (camera.x >= obj.min_x && camera.x <= obj.max_x && camera.y >= obj.min_y && camera.y <= obj.max_y && camera.z >= obj.min_z && camera.z <= obj.max_z);
}

void processMovement()
{
	//enable tour
	if (glfwGetKey(glWindow, GLFW_KEY_T)) {
		myCamera.setCameraPos(glm::vec3(9.0f, 6.45f, -10.69f));
		inTour = true;
		ok = true;
	}

	if (glfwGetKey(glWindow, GLFW_KEY_Y)) {
		inTour = false;
		ok = false;
	}

	//generate cloud
	if (glfwGetKey(glWindow, GLFW_KEY_G)) {
		if (debouncer) {
			count++;
			debouncer = false;
		}
	}

	//view point objects
	if (glfwGetKey(glWindow, GLFW_KEY_1)) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}

	//view solid objects
	if (glfwGetKey(glWindow, GLFW_KEY_2)) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	//view wireframe objects
	if (glfwGetKey(glWindow, GLFW_KEY_3)) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	//move camera up
	if (pressedKeys[GLFW_KEY_LEFT_SHIFT]) {
		if (!collision_detection(myCamera.getCameraPos() + glm::vec3(0, 1, 0) * cameraSpeed, car))
			myCamera.move(gps::MOVE_UP, cameraSpeed);
	}

	//move camera down
	if (pressedKeys[GLFW_KEY_SPACE]) {
		if (!collision_detection(myCamera.getCameraPos() - glm::vec3(0, 1, 0) * cameraSpeed, car))
			myCamera.move(gps::MOVE_DOWN, cameraSpeed);
	}

	//switch between day and night modes
	if (pressedKeys[GLFW_KEY_M]) {
		lightColor = glm::vec3(1.0f, 0.7f, 0.5f); //day light
		night = false;
	}

	//switch between day and night modes
	if (pressedKeys[GLFW_KEY_N]) {
		lightColor = glm::vec3(0.0f, 0.1f, 0.1f); //night light
		night = true;
	}

	//move camera forward
	if (pressedKeys[GLFW_KEY_W]) {
		if (!collision_detection(myCamera.getCameraPos() + myCamera.getCameraDir() * cameraSpeed, car))
			myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
	}

	//move camera left
	if (pressedKeys[GLFW_KEY_A]) {
		if (!collision_detection(myCamera.getCameraPos() - myCamera.getCameraLeftDir() * cameraSpeed, car))
			myCamera.move(gps::MOVE_LEFT, cameraSpeed);
	}

	//move camera backward
	if (pressedKeys[GLFW_KEY_S]) {
		if (!collision_detection(myCamera.getCameraPos() - myCamera.getCameraDir() * cameraSpeed, car))
			myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
	}

	//move camera right
	if (pressedKeys[GLFW_KEY_D]) {
		if (!collision_detection(myCamera.getCameraPos() + myCamera.getCameraLeftDir() * cameraSpeed, car))
			myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
	}

	//rotate directional light to the left
	if (pressedKeys[GLFW_KEY_O]) {

		lightAngle += 0.3f;
		if (lightAngle > 360.0f)
			lightAngle -= 360.0f;
		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
		myCustomShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}

	//rotate directional light to the right
	if (pressedKeys[GLFW_KEY_P]) {
		lightAngle -= 0.3f; 
		if (lightAngle < 0.0f)
			lightAngle += 360.0f;
		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
		myCustomShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}	

	//move boat forward
	if (pressedKeys[GLFW_KEY_I]) {

		if (boat_posx - cos(boat_angle) * boat_speed < 6 && boat_posx - cos(boat_angle) * boat_speed > -1 && boat_posz + sin(boat_angle) * boat_speed < 12.7 && boat_posz + sin(boat_angle) * boat_speed > 4.8) {
			boat_posx -= cos(boat_angle) * boat_speed;
			boat_posz += sin(boat_angle) * boat_speed;
		}
	}

	//rotate boat left
	if (glfwGetKey(glWindow, GLFW_KEY_J)) {
		boat_angle += 0.015f;
		wheel_angle += 2.0f;
	}

	//rotate boat right
	if (glfwGetKey(glWindow, GLFW_KEY_L)) {
		boat_angle -= 0.015f;
		wheel_angle -= 2.0f;
	}
}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	//for Mac OS X
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwMakeContextCurrent(glWindow);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);

	return true;
}

void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initFBOs()
{
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix()
{
	const GLfloat near_plane = -20.0f, far_plane = 20.0f;
	glm::mat4 lightProjection = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, near_plane, far_plane);

	glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
	glm::mat4 lightView = glm::lookAt(myCamera.getCameraTarget() + 1.0f * lightDirTr, myCamera.getCameraTarget(), glm::vec3(0.0f, 1.0f, 0.0f));

	return lightProjection * lightView;
}

void initModels()
{
	boat = gps::Model3D("objects/fisher-boat/source/boat.obj", "objects/fisher-boat/source/");
	wheel = gps::Model3D("objects/fisher-boat/source/wheel.obj", "objects/fisher-boat/source/");
	car = gps::Model3D("objects/fresh-delivery-service-car/source/car.obj", "objects/fresh-delivery-service-car/source/");
	crab = gps::Model3D("objects/dancing-crab/source/crab.obj", "objects/dancing-crab/source/");
	myModel = gps::Model3D("objects/scene/untitled.obj", "objects/scene/");
	lake = gps::Model3D("objects/lake/lake.obj", "objects/lake/");
	cloud = gps::Model3D("objects/clouds/source/cloud.obj", "objects/clouds/source/");
	lightCube = gps::Model3D("objects/cube/cube.obj", "objects/cube/");
}

void initShaders()
{
	glShadeModel(GL_SMOOTH);
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	depthMapShader.loadShader("shaders/simpleDepthMap.vert", "shaders/simpleDepthMap.frag");
}

void initUniforms()
{
	myCustomShader.useShaderProgram();

	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");

	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	
	lightDirMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDirMatrix");

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 2.0f);
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set the point light direction (direction towards the light)
	lightPos1 = glm::vec3(2.5f, -1.5f, 1.0f);
	lightPos1Loc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPos1");
	glUniform3fv(lightPos1Loc, 1, glm::value_ptr(lightPos1));

	//set the point light direction (direction towards the light)
	lightPos2 = glm::vec3(4.2f, -1.0f, -5.0f);
	lightPos2Loc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPos2");
	glUniform3fv(lightPos2Loc, 1, glm::value_ptr(lightPos2));

	//set the point light direction (direction towards the light)
	lightPos3 = glm::vec3(-2.13f, -0.8f, -3.5f);
	lightPos3Loc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPos3");
	glUniform3fv(lightPos3Loc, 1, glm::value_ptr(lightPos3));

	//set light color
	lightColor = glm::vec3(1.0f, 0.7f, 0.5f); //warm day light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	//set alpha channel
	alpha = 0.85; 
	alphaLoc = glGetUniformLocation(myCustomShader.shaderProgram, "alpha");
	glUniform1f(alphaLoc, alpha);

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	processMovement();	
	
	//render tour
	if (inTour) {
		if (i++ > 2900) {
			i = 0;
			inTour = false;
		}
		else {
			if (i < 900) {
				myCamera.setCameraDir(glm::normalize(glm::vec3(3.5f, -1.8f, 0.9f) - myCamera.getCameraPos()));
				myCamera.move(gps::MOVE_LEFT, 0.05);
			}
			else if (i < 1100) {
				myCamera.setCameraDir(glm::normalize(glm::vec3(-0.2f, -2.0f, -0.45f) - myCamera.getCameraPos()));
				myCamera.move(gps::MOVE_FORWARD, 0.05);
			}
			else {
				myCamera.setCameraDir(glm::normalize(glm::vec3(-0.2f, -2.02f, -0.45f) - myCamera.getCameraPos()));
				if (ok)
				{
					myCamera.setCameraPos(glm::vec3(-0.0f,-2.0f, -0.45f));
					ok = false;
				}
				myCamera.move(gps::MOVE_LEFT, 0.00065);
			}
		}
	}

	//render the scene to the depth buffer (first pass)
	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));
		
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	
	//create model matrix for boat
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(boat_posx, boat_posy, boat_posz));
	model = glm::rotate(model, boat_angle, glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));
	boat.Draw(depthMapShader);

	//create model matrix for wheel
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(boat_posx, boat_posy, boat_posz));
	model = glm::translate(model, glm::vec3(0.0f, -1.9f, 11.735f));
	model = glm::rotate(model, boat_angle, glm::vec3(0, 1, 0));
	model = glm::rotate(model, glm::radians(wheel_angle), glm::vec3(1, 0, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));
	wheel.Draw(depthMapShader);

	//create model matrix for crab
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(1.6f, -2.92f + crab_posy, 1.35f));
	if ((crab_angle > 16 || crab_angle < -16)) {
		ca *= -1;
		ok2 = true;
		ok1 = false;
		crab_angle += ca;
	}
	if (ok1)
		crab_angle += ca;
	if ((crab_posy > 0.3 || crab_posy < 0)) {
		cy *= -1;
		if (crab_posy < 0) {
			ok1 = true;
			ok2 = false;
			crab_posy += cy;
		}
	}
	if (ok2)
		crab_posy += cy;
	model = glm::rotate(model, glm::radians(crab_angle), glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));
	crab.Draw(depthMapShader);


	//create model matrix for scene
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));
	myModel.Draw(depthMapShader);


	//create model matrix for car
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));
	car.Draw(depthMapShader);


	//create model matrix for lake
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));

	lake.Draw(depthMapShader);

	//create clouds 
	if (debouncer) {
		for (int j = 0; j < count; j++) {
			if (cloud_posx + j * (-10.f) < 65.0f || cloud_posz + j * (-10.f) < 65.0f) {
				//create model matrix for cloud
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(cloud_posx, cloud_posy, cloud_posz));
				model = glm::translate(model, glm::vec3(j * (-10.0f), -5.0f, j * (-10.0f)));
				//send model matrix to shader
				glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
					1,
					GL_FALSE,
					glm::value_ptr(model));
				cloud.Draw(depthMapShader);
			}
		}
	}
	else {
		if (k++ > 30) {
			debouncer = true;
			k = 0;
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//render the scene (second pass)
	myCustomShader.useShaderProgram();
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
	//send lightSpace matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	//send view matrix to shader
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "view"),
		1,
		GL_FALSE,
		glm::value_ptr(view));	

	//compute light direction transformation matrix
	lightDirMatrix = glm::mat3(glm::inverseTranspose(view));
	//send lightDir matrix data to shader
	glUniformMatrix3fv(lightDirMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightDirMatrix));

	glViewport(0, 0, retina_width, retina_height);
	myCustomShader.useShaderProgram();

	//bind the depth map
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);
	

	//create model matrix for boat
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(boat_posx, boat_posy, boat_posz));
	model = glm::rotate(model, boat_angle, glm::vec3(0, 1, 0));
	//send model matrix data to shader	
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	boat.Draw(myCustomShader);


	//create model matrix for wheel
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(boat_posx, boat_posy, boat_posz));
	model = glm::translate(model, glm::vec3(0.0f, 0.4f, 0.0f));
	model = glm::rotate(model, boat_angle, glm::vec3(0, 1, 0));
	model = glm::rotate(model, glm::radians(wheel_angle), glm::vec3(1, 0, 0));
	
	//send model matrix data to shader	
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	wheel.Draw(myCustomShader);


	//create model matrix for crab
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(1.6f, -2.92f+crab_posy, 1.35f));
	if ((crab_angle > 16 || crab_angle < -16)) {
		ca *= -1;
		ok2 = true;
		ok1 = false;
		crab_angle += ca;
	}
	if (ok1)
		crab_angle += ca;
	if ((crab_posy > 0.3 || crab_posy < 0)) {
		cy *= -1;
		if (crab_posy < 0) {
			ok1 = true;
			ok2 = false;
			crab_posy += cy;
		} 
	}
	if (ok2)
		crab_posy += cy;

	model = glm::rotate(model, glm::radians(crab_angle), glm::vec3(0, 1, 0));
	//send model matrix data to shader	
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	crab.Draw(myCustomShader);


	//create model matrix for scene
	model = glm::mat4(1.0f);
	//send model matrix data to shader	
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	myModel.Draw(myCustomShader);


	//create model matrix for car
	model = glm::mat4(1.0f);
	//send model matrix data to shader	
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	car.Draw(myCustomShader);


	//enable blending for transparent objects
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//create model matrix for lake
	model = glm::mat4(1.0f);
	//send model matrix data to shader	
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	//set alpha channel
	alpha = 0.85;
	alphaLoc = glGetUniformLocation(myCustomShader.shaderProgram, "alpha");
	glUniform1f(alphaLoc, alpha);

	lake.Draw(myCustomShader);


	if (debouncer) {
		for (int j = 0; j < count; j++) {
			if (cloud_posx + j * (-10.f) < 65.0f || cloud_posz + j * (-10.f) < 65.0f) {
				//create model matrix for cloud
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(cloud_posx, cloud_posy, cloud_posz));
				model = glm::translate(model, glm::vec3(j * (-10.0f), -5.0f, j * (-10.0f)));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

				//compute normal matrix
				normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
				//send normal matrix data to shader
				glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

				//set alpha channel
				alpha = 0.5;
				alphaLoc = glGetUniformLocation(myCustomShader.shaderProgram, "alpha");
				glUniform1f(alphaLoc, alpha);

				cloud.Draw(myCustomShader);
				started = true;
			}
		}
		if (started) {
			cloud_posx += cloud_speed;
			cloud_posz += cloud_speed;
			started = false;
		}
	}
	else {
		if (k++ > 30) {
			debouncer = true;
			k = 0;
		}
	}

	//disable blending
	glDisable(GL_BLEND);

	if (!night) {

		//draw a white cube around the light
		lightShader.useShaderProgram();
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

		model = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::translate(model, 1.0f * lightDir);
		model = glm::scale(model, glm::vec3(0.005f, 0.005f, 0.005f));
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		lightCube.Draw(lightShader);
	}
}

int main(int argc, const char * argv[]) {

	initOpenGLWindow();
	initOpenGLState();
	initFBOs();
	initModels();
	initShaders();
	initUniforms();	
	glCheckError();

	while (!glfwWindowShouldClose(glWindow)) {
		renderScene();

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	//close GL context and any other GLFW resources
	glfwTerminate();

	return 0;
}
