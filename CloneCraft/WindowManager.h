#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class WindowManager {
public:
	WindowManager();
	~WindowManager();
	void CreateWindow(int WINDOW_WIDTH, int WINDOW_HEIGHT, const char* WINDOW_TITLE);

private:
	GLFWwindow* window;
	bool framebufferResized = false;

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto windowManager = reinterpret_cast<WindowManager*>(glfwGetWindowUserPointer(window));
		windowManager->framebufferResized = true;
	}
};

