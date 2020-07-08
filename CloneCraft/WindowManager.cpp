#include "WindowManager.h"

WindowManager::WindowManager() {
}

WindowManager::~WindowManager() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void WindowManager::CreateWindow(uint32_t WINDOW_WIDTH, uint32_t WINDOW_HEIGHT, const char* WINDOW_TITLE) {
	if (glfwInit() == GLFW_TRUE) {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	};
}

GLFWwindow* WindowManager::GetWindow() {
	return window;
}

bool WindowManager::isFramebufferResized() {
	return framebufferResized;
}

void WindowManager::SetFramebufferResized(bool value) {
	framebufferResized = value;
}

void WindowManager::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto windowManager = reinterpret_cast<WindowManager*>(glfwGetWindowUserPointer(window));
	windowManager->framebufferResized = true;
}
