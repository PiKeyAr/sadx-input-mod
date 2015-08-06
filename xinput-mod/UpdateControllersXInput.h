#pragma once

#include "typedefs.h"
#include <Xinput.h>

// Re-defining so it can be changed easily, and because XUSER_MAX_COUNT is far too long.
#define XPAD_COUNT XUSER_MAX_COUNT

namespace xinput
{
	enum Motor : int8
	{
		None,
		Left,
		Right,
		Both
	};

	struct Settings
	{
		Settings();
		// Left stick deadzone
		short	deadzoneL;
		bool	normalizeL;
		// Right stick deadzone
		short	deadzoneR;
		bool	normalizeR;
		// Trigger threshold
		ushort	triggerThreshold;
		// Rumble multiplier (1.0 by default)
		float	rumbleMultiplier;

		void apply(short deadzoneL, short deadzoneR, bool normalizeL, bool normalizeR, ushort triggerThreshold, float rumbleMultiplier);
	};

	extern Settings settings[XPAD_COUNT];

	// Ingame functions
	void __cdecl UpdateControllersXInput();
	void __cdecl RumbleLarge(int playerNumber, signed int intensity);
	void __cdecl RumbleSmall(int playerNumber, signed int a2, signed int a3, int a4);
	void Rumble(short id, int a1, Motor motor);

	// Utility functions
	// TODO: Separate utility functions into separate file.
	void ConvertAxes(short dest[2], short source[2], short deadzone = 0, bool normalize = true);
	int ConvertButtons(ushort id, XINPUT_GAMEPAD* xpad);
	static void SetMotor(ushort id, Motor motor, short intensity);
}
