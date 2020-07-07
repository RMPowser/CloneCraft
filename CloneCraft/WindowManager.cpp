#include "WindowManager.h"

WindowManager::WindowManager() {
}

WindowManager::~WindowManager() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void WindowManager::CreateWindow(int WINDOW_WIDTH, int WINDOW_HEIGHT, const char* WINDOW_TITLE) {
	if (glfwInit() == GLFW_TRUE) {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	};
}
