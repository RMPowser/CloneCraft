#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "CloneCraftApp.h"
#include "Block.h"

int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	AppConfig config;
	config.windowTitle = "CloneCraft   :^)";

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(config.windowX, config.windowY, config.windowTitle.c_str(), nullptr, nullptr);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	glfwSetKeyCallback(window, handleKeyboardInput);
	glfwSetCursorPosCallback(window, handleMouseInput);
	glfwSetMouseButtonCallback(window, handleMouseButtonInput);

			#ifndef NDEBUG
			if (+vulkan.Create(window, 0, config.validationLayers.size(), config.validationLayers.data(), 0, nullptr, config.deviceExtensions.size(), config.deviceExtensions.data(), true))
			#else
			if (+vulkan.Create(window, 0))
			#endif
			{
				Renderer renderer(window, vulkan, config);
				VkClearValue clearValue;
				clearValue.color = { (70.0f / 255), (160.0f / 255), (255.0f / 255), (255.0f / 255) };

				static auto startTime = std::chrono::high_resolution_clock::now();
				while (+window.ProcessWindowEvents()) {
					auto currentTime = std::chrono::high_resolution_clock::now();
					float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
					startTime = std::chrono::high_resolution_clock::now();

					if (+vulkan.StartFrame(1, &clearValue.color)) {
						renderer.Render(time);
						vulkan.EndFrame(true);
					}
				}
			}
		}
	
	} catch (const std::exception& e) {
		printf("\n%s\n", e.what());
		system("pause");
		return EXIT_FAILURE;
	}

	printf("\n");
	system("pause");
	return EXIT_SUCCESS;
}