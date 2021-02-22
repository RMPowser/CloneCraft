#pragma once

class Window {
public:
	GW::SYSTEM::GWindow gWindow;
	GW::GRAPHICS::GVulkanSurface vulkan;

	Window() {
		if (-gWindow.Create(0, 0, 800, 600, GW::SYSTEM::GWindowStyle::WINDOWEDBORDERED)) {
			throw std::exception("Failed to create GWindow!");
		}

		gWindow.SetWindowName(AppGlobals::windowTitle.c_str());

		unsigned long long bitmask = GW::GRAPHICS::GGraphicsInitOptions::DEPTH_BUFFER_SUPPORT;

#ifndef NDEBUG
		if (-vulkan.Create(gWindow, bitmask, AppGlobals::validationLayers.size(), AppGlobals::validationLayers.data(), 0, nullptr, AppGlobals::deviceExtensions.size(), AppGlobals::deviceExtensions.data(), true)) {
#else
		if (-vulkan.Create(gWindow, bitmask)) {
#endif
			throw std::exception("Failed to create Vulkan surface!");
		}
	}

	bool IsFocus() {
		bool b;
		gWindow.IsFocus(b);
		return b;
	}

	bool IsFullscreen() {
		bool b;
		gWindow.IsFullscreen(b);
		return b;
	}

	bool ProcessWindowEvents() {
		return +gWindow.ProcessWindowEvents() ? true : false;
	}

	bool ReconfigureWindow(int x, int y, unsigned int width, unsigned int height, GW::SYSTEM::GWindowStyle style) {
		return +gWindow.ReconfigureWindow(x, y, width, height, style) ? true : false;
	}

	bool SetWindowName(const char* newName) {
		return +gWindow.SetWindowName(newName) ? true : false;
	}

	bool SetIcon(int width, int height, const unsigned int* argbPixels) {
		return +gWindow.SetIcon(width, height, argbPixels) ? true : false;
	}

	bool MoveWindow(int x, int y) {
		return +gWindow.MoveWindow(x, y) ? true : false;
	}

	bool ResizeWindow(int width, int height) {
		return +gWindow.ResizeWindow(width, height) ? true : false;
	}

	bool Maximize() {
		return +gWindow.Maximize() ? true : false;
	}

	bool Minimize() {
		return +gWindow.Minimize() ? true : false;
	}

	unsigned int GetWidth() {
		unsigned int w;
		gWindow.GetWidth(w);
		return w;
	}

	unsigned int GetHeight() {
		unsigned int h;
		gWindow.GetHeight(h);
		return h;
	}

	unsigned int GetClientWidth() {
		unsigned int w;
		gWindow.GetClientWidth(w);
		return w;
	}

	unsigned int GetClientHeight() {
		unsigned int h;
		gWindow.GetClientHeight(h);
		return h;
	}

	unsigned int GetX() {
		unsigned int x;
		gWindow.GetX(x);
		return x;
	}

	unsigned int GetY() {
		unsigned int y;
		gWindow.GetY(y);
		return y;
	}

	POINT GetClientTopLeft() {
		POINT v;
		gWindow.GetClientTopLeft(*(unsigned int*)&v.x, *(unsigned int*)&v.y);
		return v;
	}
};