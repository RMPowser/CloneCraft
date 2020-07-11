#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class WindowManager {
public:
	WindowManager();
	~WindowManager();
	void CreateWindow(uint32_t WINDOW_WIDTH, uint32_t WINDOW_HEIGHT, const char* WINDOW_TITLE);

	GLFWwindow*		GetWindow()							{ return window; }
	bool			isFramebufferResized()				{ return framebufferResized; }
	void			SetFramebufferResized(bool value)	{ framebufferResized = value; }

private:
	GLFWwindow* window;
	bool framebufferResized = false;

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};

