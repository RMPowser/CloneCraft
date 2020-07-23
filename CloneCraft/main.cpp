#include "CloneCraftApp.h"
#include "Block.h"

int main() {
	AppConfig config(800, 600, 75, "CloneCraft   :^)", 2, { VK_KHR_SWAPCHAIN_EXTENSION_NAME });

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(config.windowX, config.windowY, config.windowTitle.c_str(), nullptr, nullptr);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetKeyCallback(window, handleKeyboardInput);
	glfwSetCursorPosCallback(window, handleMouseInput);

	CloneCraftApplication app(window, &config);

	try {
		app.run();
	} catch (const std::exception& e) {
		printf("%s\n", e.what());
		system("pause");
		return EXIT_FAILURE;
	}

	system("pause");
	return EXIT_SUCCESS;
}