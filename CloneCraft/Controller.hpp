#pragma once

struct Controller {
	bool forwardPressed = false;
	bool leftPressed = false;
	bool rightPressed = false;
	bool backPressed = false;
	bool upPressed = false;
	bool downPressed = false;
	bool flyToggleNew = false;
	bool flyToggleOld = false;
	bool speedModifierPressed = false;
	bool leftClicked = false;
	bool rightClicked = false;
	bool firstMouse = true;
	double lastMouseX = 0;
	double lastMouseY = 0;
};