#define GLM_ENABLE_EXPERIMENTAL
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <vector>
#include <algorithm>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

#include "tools/texture.hpp"
#include "Particle.hpp"
#include "Camera.hpp"
#include "Shader.hpp"
#include "Skybox.hpp"
#include "Model.hpp"
#include "Water.hpp"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Pointer only, object tạo trong main
Camera* camera = nullptr;
Camera* pDownCam = nullptr;
Camera* pUpCam = nullptr;

Model* p_ship;

float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float rotate_offset = 0.f;
int rotate_dir = -1;
float max_left = 5;

float height_offset = 0.f;
int height_dir = -1;
float max_down = 0.2f;

int main(void)
{
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Waterfall", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window.\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwPollEvents();
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	// cảng tàu
	Shader simpleBoxShader("shader/simple.vs", "shader/simple.fs");

	float cubeVertices[] = {
	-0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,0.5f,-0.5f,
	 0.5f,0.5f,-0.5f, -0.5f,0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
	-0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f,0.5f, 0.5f,
	 0.5f,0.5f, 0.5f, -0.5f,0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
	-0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
	-0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
	 0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
	 0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
	-0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
	 0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,
	-0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
	 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f
	};

	unsigned int cubeVAO, cubeVBO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);

	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	// ====================== Cameras
	glm::vec3 upCenter(0.f, 20.f, 1.f);
	Camera upCamera(upCenter, glm::vec3(0.f, 1.f, 0.f), -268.6f, -80.5f);
	Camera downCamera(glm::vec3(0.f, 3.f, 15.f));
	camera = &upCamera;
	pDownCam = &downCamera;
	pUpCam = &upCamera;

	// Gán con trỏ camera tĩnh để dùng với SPACE
	static Camera* pDownCam = &downCamera;
	static Camera* pUpCam = &upCamera;

	// ====================== Skybox
	float skyboxScale = 500.f;
	Shader skyboxShader("shader/skybox.vs", "shader/skybox.fs");
	Skybox skybox(skyboxScale, &skyboxShader);

	// ====================== Particle
	Shader particleShader("shader/particle.vs", "shader/particle.fs");
	Particles waterfall(glm::vec3(2.f, 4.f, 10.f), &particleShader);

	// ====================== Model
	Shader modelShader("shader/model.vs", "shader/model.fs");

	Model mountain("material/mountain/plane-7.obj", &modelShader);
	mountain.ModelMatrix = glm::scale(mountain.ModelMatrix, glm::vec3(1 / 70.f, 1 / 50.f, 1 / 70.f));

	Model ship("material/ship/ShipMoscow.obj", &modelShader);
	p_ship = &ship;
	ship.ModelMatrix = glm::translate(ship.ModelMatrix, glm::vec3(0, 0, 8.f));
	ship.ModelMatrix = glm::rotate(ship.ModelMatrix, glm::radians(-90.f), glm::vec3(1, 0, 0));
	ship.ModelMatrix = glm::rotate(ship.ModelMatrix, glm::radians(-40.f), glm::vec3(0, 0, 1));
	ship.ModelMatrix = glm::scale(ship.ModelMatrix, glm::vec3(0.1f, 0.1f, 0.1f));
	ship.ModelMatrix = glm::translate(ship.ModelMatrix, glm::vec3(50.f, 0.f, 17.3f));

	Model ship2("material/ship2/ship2.obj", &modelShader);
	ship2.ModelMatrix = glm::mat4(1.0f);
	ship2.ModelMatrix = glm::scale(ship2.ModelMatrix, glm::vec3(0.008f));
	ship2.ModelMatrix = glm::rotate(ship2.ModelMatrix, glm::radians(90.f), glm::vec3(0, 1, 0));
	ship2.ModelMatrix = glm::translate(ship2.ModelMatrix, glm::vec3(-150.f, 190.f, 200.f));

	// ====================== Water
	Shader waterShader("shader/water.vs", "shader/water.fs");
	Water water(&waterShader);
	water.ModelMatrix = glm::scale(water.ModelMatrix, glm::vec3(2.f, 2.f, 2.f));
	water.ModelMatrix = glm::rotate(water.ModelMatrix, glm::radians(90.f), glm::vec3(0, 1, 0));
	water.ModelMatrix = glm::translate(water.ModelMatrix, glm::vec3(-10.f, 0.8f, -6.5f));

	waterShader.use();
	glm::vec3 lightPos(0.0f, -20.0f, 2.0f);
	GLfloat materAmbient[] = { 0.1, 0.1, 0.3, 1.0 };
	GLfloat materSpecular[] = { 0.8, 0.8, 0.9, 1.0 };
	GLfloat lightDiffuse[] = { 0.7, 0.7, 0.8, 1.0 };
	GLfloat lightAmbient[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat lightSpecular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat envirAmbient[] = { 0.1, 0.1, 0.3, 1.0 };

	waterShader.setVec3("lightPos", lightPos);
	waterShader.setVec3("viewPos", camera->Position);
	glUniform4fv(glGetUniformLocation(waterShader.ID, "materAmbient"), 1, materAmbient);
	glUniform4fv(glGetUniformLocation(waterShader.ID, "materSpecular"), 1, materSpecular);
	glUniform4fv(glGetUniformLocation(waterShader.ID, "lightDiffuse"), 1, lightDiffuse);
	glUniform4fv(glGetUniformLocation(waterShader.ID, "lightAmbient"), 1, lightAmbient);
	glUniform4fv(glGetUniformLocation(waterShader.ID, "lightSpecular"), 1, lightSpecular);
	glUniform4fv(glGetUniformLocation(waterShader.ID, "envirAmbient"), 1, envirAmbient);

	while (!glfwWindowShouldClose(window))
	{
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// set the frame
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		processInput(window);

		// MVP matrix
		glm::mat4 ViewMatrix = camera->GetViewMatrix();
		glm::mat4 ProjectionMatrix = glm::perspective(
			glm::radians(camera->Zoom),
			(float)SCR_WIDTH / (float)SCR_HEIGHT,
			0.1f,
			100.0f
		);
		glm::mat4 ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		glm::vec3 CameraPosition(glm::inverse(ViewMatrix)[3]);

		//======================
		// water render
		//======================

		water.UpdateWave(currentFrame);
		water.draw(ViewMatrix, ProjectionMatrix, currentFrame);

		//======================
		// skybox render
		//======================

		skybox.draw(ViewProjectionMatrix);

		//======================
		// model render
		//======================

		mountain.draw(ViewProjectionMatrix);
		ship.draw(ViewProjectionMatrix);
		ship2.draw(ViewProjectionMatrix);

		//======================
		// particle render
		//======================

		int newparticles = (int)(deltaTime * 1000.0);
		if (newparticles > (int)(0.016f * 10000.0))
			newparticles = (int)(0.016f * 10000.0);
		waterfall.SpawnParticles(currentFrame, newparticles);
		waterfall.UpdateParticles(deltaTime, CameraPosition);
		waterfall.draw(ViewMatrix, ViewProjectionMatrix);

		//======================
		// render end
		//======================

		// ====================== Draw Harbor Dock & Accessories
		simpleBoxShader.use();
		simpleBoxShader.setMat4("u_view", ViewMatrix);
		simpleBoxShader.setMat4("u_projection", ProjectionMatrix);
		simpleBoxShader.setVec3("u_color", glm::vec3(0.4f, 0.3f, 0.2f));
		glBindVertexArray(cubeVAO);

		// === Dock Platform
		glm::vec3 dockOrigin = glm::vec3(-5.f, 2.f, 10.3f);  // gốc cảng
		simpleBoxShader.setVec3("u_color", glm::vec3(0.4f, 0.3f, 0.2f));
		glm::mat4 dock = glm::mat4(1.0f);
		dock = glm::translate(dock, dockOrigin);
		dock = glm::scale(dock, glm::vec3(10.0f, 0.1f, 3.0f));
		simpleBoxShader.setMat4("u_model", dock);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// === Dock Pillars: chia ở 4 góc và 2 mép giữa
		std::vector<glm::vec3> pillarOffsets = {
			{-5.f, -2.f, -1.5f}, // góc trái trước
			{-5.f, -2.f,  1.5f}, // góc trái sau
			{ 5.f, -2.f, -1.5f}, // góc phải trước
			{ 5.f, -2.f,  1.5f}, // góc phải sau
			{ 0.f, -2.f, -1.5f}, // giữa trước
			{ 0.f, -2.f,  1.5f}  // giữa sau
		};

		simpleBoxShader.setVec3("u_color", glm::vec3(0.3f, 0.2f, 0.1f)); // màu nâu gỗ
		for (auto offset : pillarOffsets) {
			glm::mat4 pillar = glm::mat4(1.0f);
			pillar = glm::translate(pillar, dockOrigin + offset);
			pillar = glm::scale(pillar, glm::vec3(0.2f, 4.0f, 0.2f));
			simpleBoxShader.setMat4("u_model", pillar);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		// === Railing posts
		int numPosts = 6;
		float postSpacing = 10.0f / (numPosts - 1); // chia đều từ -5 đến +5

		simpleBoxShader.setVec3("u_color", glm::vec3(0.9f, 0.9f, 0.9f)); // màu rào

		for (int i = 0; i < numPosts; ++i) {
			float x = -5.0f + i * postSpacing;

			// Cột rào bên phải (Z = -1.3f)
			glm::mat4 postRight = glm::mat4(1.0f);
			postRight = glm::translate(postRight, dockOrigin + glm::vec3(x, 0.6f, -1.3f));
			postRight = glm::scale(postRight, glm::vec3(0.1f, 1.2f, 0.1f));
			simpleBoxShader.setMat4("u_model", postRight);
			glDrawArrays(GL_TRIANGLES, 0, 36);

			// Cột rào bên trái (Z = +1.3f)
			glm::mat4 postLeft = glm::mat4(1.0f);
			postLeft = glm::translate(postLeft, dockOrigin + glm::vec3(x, 0.6f, 1.3f));
			postLeft = glm::scale(postLeft, glm::vec3(0.1f, 1.2f, 0.1f));
			simpleBoxShader.setMat4("u_model", postLeft);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		// === Railing bar
		// Thanh nối rào bên phải
		glm::mat4 railRight = glm::mat4(1.0f);
		railRight = glm::translate(railRight, dockOrigin + glm::vec3(0, 1.1f, -1.3f));
		railRight = glm::scale(railRight, glm::vec3(10.0f, 0.05f, 0.05f));
		simpleBoxShader.setVec3("u_color", glm::vec3(0.9f, 0.9f, 0.9f));
		simpleBoxShader.setMat4("u_model", railRight);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// Thanh nối rào bên trái
		glm::mat4 railLeft = glm::mat4(1.0f);
		railLeft = glm::translate(railLeft, dockOrigin + glm::vec3(0, 1.1f, 1.3f));
		railLeft = glm::scale(railLeft, glm::vec3(10.0f, 0.05f, 0.05f));
		simpleBoxShader.setMat4("u_model", railLeft);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// === Light poles & heads
		int numLights = 4;
		float lightSpacing = 10.0f / (numLights - 1);

		for (int i = 0; i < numLights; ++i) {
			float x = -5.0f + i * lightSpacing;

			for (float zOffset : { -1.5f, 1.5f }) { // hai bên trái/phải
				// Cột đèn
				glm::mat4 pole = glm::mat4(1.0f);
				pole = glm::translate(pole, dockOrigin + glm::vec3(x, 1.5f, zOffset));
				pole = glm::scale(pole, glm::vec3(0.1f, 3.0f, 0.1f));
				simpleBoxShader.setVec3("u_color", glm::vec3(0.2f, 0.2f, 0.2f));
				simpleBoxShader.setMat4("u_model", pole);
				glDrawArrays(GL_TRIANGLES, 0, 36);

				// Bóng đèn
				glm::mat4 head = glm::mat4(1.0f);
				head = glm::translate(head, dockOrigin + glm::vec3(x, 3.1f, zOffset));
				head = glm::scale(head, glm::vec3(0.4f));
				simpleBoxShader.setVec3("u_color", glm::vec3(1.0f, 1.0f, 0.6f));
				simpleBoxShader.setMat4("u_model", head);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		extern Camera* pDownCam;
		extern Camera* pUpCam;

		if (pDownCam == nullptr || pUpCam == nullptr)
			return; // chưa được gán

		if (camera == pDownCam)
			camera = pUpCam;
		else
			camera = pDownCam;
	}

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera->ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera->ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera->ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera->ProcessKeyboard(RIGHT, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
		camera->ProcessKeyboard(UPWORD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
		camera->ProcessKeyboard(DOWNWORD, deltaTime);

	glm::vec3 ship_front(1.f, 0, 0);
	const float translate_v = 5.f;
	const float turn_v = 0.2f;
	const float rotate_v = 0.1f;
	const float float_v = 0.005f;

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
	{
		p_ship->ModelMatrix = glm::translate(p_ship->ModelMatrix, deltaTime * (-translate_v) * ship_front);
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		p_ship->ModelMatrix = glm::translate(p_ship->ModelMatrix, deltaTime * translate_v * ship_front);
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		if (rotate_offset != 0)
		{
			p_ship->ModelMatrix = glm::rotate(p_ship->ModelMatrix, glm::radians(-rotate_offset), glm::vec3(1, 0, 0));
			rotate_offset = 0;
		}
		p_ship->ModelMatrix = glm::translate(p_ship->ModelMatrix, deltaTime * (-translate_v) * ship_front * 0.4f);
		p_ship->ModelMatrix = glm::rotate(p_ship->ModelMatrix, turn_v * deltaTime, glm::vec3(0, 0, 1));
	}
	else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		if (rotate_offset != 0)
		{
			p_ship->ModelMatrix = glm::rotate(p_ship->ModelMatrix, glm::radians(-rotate_offset), glm::vec3(1, 0, 0));
			rotate_offset = 0;
		}
		p_ship->ModelMatrix = glm::translate(p_ship->ModelMatrix, deltaTime * (-translate_v) * ship_front * 0.4f);
		p_ship->ModelMatrix = glm::rotate(p_ship->ModelMatrix, -turn_v * deltaTime, glm::vec3(0, 0, 1));
	}
	else
	{
		if ((rotate_offset < -max_left && rotate_dir == -1) || (rotate_offset > max_left && rotate_dir == 1))
		{
			max_left = (rand() % 1000 / 1000.f) * 8.f + 3.f;
			rotate_dir = -rotate_dir;
		}

		float single_rotate = (rand() % 2 + 1) * rotate_dir * rotate_v;
		rotate_offset += single_rotate;
		p_ship->ModelMatrix = glm::rotate(p_ship->ModelMatrix, glm::radians(single_rotate), glm::vec3(1, 0, 0));

		if ((height_offset < -0.2) || (height_offset > 0.2))
			height_dir = -height_dir;

		float single_translate = height_dir * float_v;
		height_offset += single_translate;
		p_ship->ModelMatrix = glm::translate(p_ship->ModelMatrix, glm::vec3(0, 0, single_translate));
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (!camera) return;
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera->ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera->ProcessMouseScroll(yoffset);
}