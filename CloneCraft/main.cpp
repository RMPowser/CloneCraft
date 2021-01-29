#ifndef NDEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#define GATEWARE_ENABLE_CORE
#define GATEWARE_ENABLE_SYSTEM
#define GATEWARE_ENABLE_GRAPHICS
#define GATEWARE_ENABLE_AUDIO
#define GATEWARE_ENABLE_MATH
#define GATEWARE_ENABLE_INPUT
#include "../Gateware/Gateware.h"
#include "AppGlobals.hpp"
#include "Renderer.hpp"

void PrintDebugInfo(const char* string)
{
	#ifndef NDEBUG
	std::cout << string << '\n';
	#endif
}

int main() {
	#ifndef NDEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	#endif

	GW::SYSTEM::GWindow window;
	GW::CORE::GEventReceiver windowEvents;
	GW::GRAPHICS::GVulkanSurface vulkan;

	try {
		PrintDebugInfo("Creating GWindow...");
		if (+window.Create(200, 200, AppGlobals::windowX, AppGlobals::windowY, GW::SYSTEM::GWindowStyle::WINDOWEDBORDERED)) {
			PrintDebugInfo("Setting GWindow title...");
			window.SetWindowName(AppGlobals::windowTitle.c_str());
			windowEvents.Create(window, [&]() {});

			unsigned long long bitmask = GW::GRAPHICS::GGraphicsInitOptions::DEPTH_BUFFER_SUPPORT;

			PrintDebugInfo("Creating GVulkanSurface...");
			#ifndef NDEBUG
			if (+vulkan.Create(window, bitmask, AppGlobals::validationLayers.size(), AppGlobals::validationLayers.data(), 0, nullptr, AppGlobals::deviceExtensions.size(), AppGlobals::deviceExtensions.data(), true))
			#else
			if (+vulkan.Create(window, bitmask))
			#endif
			{
				Renderer renderer(window, vulkan);

				std::array<VkClearValue, 2> clearValues{};
				clearValues[0].color = { (70.0f / 255), (160.0f / 255), (255.0f / 255), (255.0f / 255) };
				clearValues[1].depthStencil = { 1.0f, 0 };

				static auto startTime = std::chrono::high_resolution_clock::now();
				while (+window.ProcessWindowEvents()) {
					auto currentTime = std::chrono::high_resolution_clock::now();
					float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
					startTime = std::chrono::high_resolution_clock::now();

					renderer.Update(time);
					if (+vulkan.StartFrame(clearValues.size(), clearValues.data())) {
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

	return EXIT_SUCCESS;
}