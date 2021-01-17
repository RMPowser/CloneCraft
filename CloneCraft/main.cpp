#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define GATEWARE_ENABLE_CORE
#define GATEWARE_ENABLE_SYSTEM
#define GATEWARE_ENABLE_GRAPHICS
#define GATEWARE_ENABLE_AUDIO
#define GATEWARE_ENABLE_MATH
#define GATEWARE_ENABLE_INPUT
#include "Gateware.h"
#include "AppConfig.h"
#include "Renderer.h"

int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	AppConfig config;
	config.windowTitle = "CloneCraft   :^)";
	config.renderDistance = 8;

	GW::SYSTEM::GWindow window;
	GW::CORE::GEventReceiver events;
	GW::GRAPHICS::GVulkanSurface vulkan;

	try {
		if (+window.Create(200, 200, config.windowX, config.windowY, GW::SYSTEM::GWindowStyle::WINDOWEDBORDERED)) {
			window.SetWindowName(config.windowTitle.c_str());
			events.Create(window, [&]() {});

			unsigned long long bitmask = GW::GRAPHICS::GGraphicsInitOptions::DEPTH_BUFFER_SUPPORT;

			#ifndef NDEBUG
			if (+vulkan.Create(window, bitmask, config.validationLayers.size(), config.validationLayers.data(), 0, nullptr, config.deviceExtensions.size(), config.deviceExtensions.data(), true))
			#else
			if (+vulkan.Create(window, bitmask))
			#endif
			{
				Renderer renderer(window, vulkan, config);

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