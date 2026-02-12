/*
 * Copyright (c) 2025  The UniversalVision project authors. MIT License. All Rights Reserved.
 *
 * This file is part of UniversalVision(https://github.com/wyewyewye/UniversalVision).
 *
 * See LICENSE file for full terms.
 */

#include <iostream>
#include <string>
#include "application.h"
#include "base/inc/logging.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"

static std::string TAG = "main";

int main(int argc, char* argv[])
{
	univision::initLog();
	LOG_INFO(TAG) << "wye test info log";
	system("pause");
	return 0;
	std::cout << "Hello, World!" << std::endl;
	// Init opengl core mode.
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	Application::PrintHelloWorld();

	// Create window.
	GLFWwindow* window = glfwCreateWindow(800, 600, "UniversalVision", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Check glad.
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}