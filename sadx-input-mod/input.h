#pragma once

#include "DreamPad.h"

namespace input
{
	void poll_controllers();
	void WriteAnalogs_hook();
	void InitRawControllers_hook();
	void __cdecl EnableController_r(Uint8 index);
	void __cdecl DisableController_r(Uint8 index);

	extern ControllerData raw_input[GAMEPAD_COUNT];
	extern bool controller_enabled[GAMEPAD_COUNT];
	extern bool debug;
	extern bool mouse_disabled;
}

void ClearVanillaSADXKeys(bool force);
int TranslateSteamToWindows(int steamkey);
char GetEKey(char index);
extern bool e_key[9];
extern __int16 Demo_Frame;
extern FullConfig_Pad PlayerConfigs[];