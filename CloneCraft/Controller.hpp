#pragma once
#include <unordered_map>

class Controller {
	GW::INPUT::GInput input;

	void GetNewClientCenter() {
		Window& window = AppGlobals::window;
		auto w = window.GetClientWidth();
		auto h = window.GetClientHeight();
		auto x = window.GetClientTopLeftX();
		auto y = window.GetClientTopLeftY();

		clientCenter.x = (w / 2) + x;
		clientCenter.y = (h / 2) + y;
	}

	void SetCursorPositionToClientCenter() {
		GetNewClientCenter();
		SetCursorPos((int)clientCenter.x , (int)clientCenter.y );
	}

public:
	std::unordered_map<int, bool> keys;
	Vec2 clientCenter{ 0, 0 };
	Vec2 mouseDelta{ 0, 0 };
	bool flyToggle = false;
	bool firstMouse = true;
	float horizontalAxis = 0; // left/right axis
	float forwardAxis = 0; // forward/backward axis
	float verticalAxis = 0; // up/down axis

	Controller() {
		Window& window = AppGlobals::window;

		if (-input.Create(window.gWindow)) {
			throw std::exception("Controller creation failed!");
		}
	}

	bool GetKey(int keyCode) {
		float outstate;
		input.GetState(keyCode, outstate);
		return outstate > 0 ? true : false;
	}

	void update() {
		keys[G_KEY_W] = GetKey(G_KEY_W);
		keys[G_KEY_A] = GetKey(G_KEY_A);
		keys[G_KEY_S] = GetKey(G_KEY_S);
		keys[G_KEY_D] = GetKey(G_KEY_D);
		keys[G_KEY_F] = GetKey(G_KEY_F);
		keys[G_KEY_SPACE] = GetKey(G_KEY_SPACE);
		keys[G_KEY_CONTROL] = GetKey(G_KEY_CONTROL);
		keys[G_KEY_LEFTSHIFT] = GetKey(G_KEY_LEFTSHIFT);
		keys[G_BUTTON_LEFT] = GetKey(G_BUTTON_LEFT);
		keys[G_BUTTON_RIGHT] = GetKey(G_BUTTON_RIGHT);

		// set axis
		horizontalAxis = 0;
		forwardAxis = 0;
		verticalAxis = 0;
		if (keys[G_KEY_A]) { horizontalAxis += 1; }
		if (keys[G_KEY_D]) { horizontalAxis -= 1; }
		if (keys[G_KEY_W]) { forwardAxis -= 1; }
		if (keys[G_KEY_S]) { forwardAxis += 1; }
		if (keys[G_KEY_SPACE]) { verticalAxis += 1; }
		if (keys[G_KEY_CONTROL]) { verticalAxis -= 1; }

				

		if (firstMouse) {
			firstMouse = false;
			SetCursorPositionToClientCenter();
			return;
		}

		POINT p;
		GetCursorPos(&p);
		mouseDelta.x = p.x - clientCenter.x;
		mouseDelta.y = p.y - clientCenter.y;
		SetCursorPositionToClientCenter();
	}
};