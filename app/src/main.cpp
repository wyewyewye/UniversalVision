#include <iostream>
#include "application.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"

int main(int argc, char* argv[])
{
	std::cout << "Hello, World!" << std::endl;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);


	return 0;
}