#ifndef NDEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "AppGlobals.hpp"
#include "Renderer.hpp"

int main() {
	#ifndef NDEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	#endif

	auto& window = AppGlobals::window;

	try {
		Renderer renderer;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { (70.0f / 255), (160.0f / 255), (255.0f / 255), (255.0f / 255) };
		clearValues[1].depthStencil = { 1.0f, 0 };

		static auto startTime = std::chrono::high_resolution_clock::now();
		while (window.ProcessWindowEvents()) {
			if (window.IsFocus()) {
				while(ShowCursor(false) >= 0);

				auto currentTime = std::chrono::high_resolution_clock::now();
				float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
				startTime = std::chrono::high_resolution_clock::now();

				if (time < 0.1f) {
					renderer.update(time);
					if (+window.vulkan.StartFrame(clearValues.size(), clearValues.data())) {
						renderer.Render(time);
						window.vulkan.EndFrame(true);
					}
				}
			}
			else {
				while (ShowCursor(true) < 0);
			}
		}
	
	} catch (const std::exception& e) {
		printf("\n%s\n", e.what());
		system("pause");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}