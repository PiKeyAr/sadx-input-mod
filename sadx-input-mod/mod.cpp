#include "stdafx.h"
#include <Windows.h>
#include <direct.h>	// for _getcwd

#include <string>
#include <map>
#include <sstream>	// because

#include "SDL.h"
#include "tinyxml.h"

#include <SADXModLoader.h>
#include <IniFile.hpp>

#include "typedefs.h"
#include "FileExists.h"
#include "input.h"
#include "rumble.h"
#include "SteamStuff.h"

using namespace std;

XMLBindings XMLConfig[10];
FullConfig_Pad PlayerConfigs[10];

FullConfig_Pad DefaultKeyboardConfig = {
	7849,
	8689,
	true,
	false,
	30,
	1.0f,
	false,
	0,
	{ PadItemType_Button, VK_UP, 0 },
	{ PadItemType_Button, VK_DOWN, 0 },
	{ PadItemType_Button, VK_LEFT, 0 },
	{ PadItemType_Button, VK_RIGHT, 0 },
	{ PadItemType_Button, 'I', 0 },
	{ PadItemType_Button, 'M', 0 },
	{ PadItemType_Button, 'J', 0 },
	{ PadItemType_Button, 'L', 0 },
	{ PadItemType_Button, VK_RETURN, 0 },
	{ PadItemType_Button, 'X', 0 },
	{ PadItemType_Button, 'Z', 0 },
	{ PadItemType_Button, 'A', 0 },
	{ PadItemType_Button, 'S', 0 },
	{ PadItemType_Button, 'Q', 0 },
	{ PadItemType_Button, 'W', 0 },
	{ PadItemType_Button, 'B', 0 },
	{ PadItemType_Button, 'C', 0 },
	{ PadItemType_Button, 'V', 0 },
	{ PadItemType_Button, 'E', 0 },
	{ PadItemType_Button, VK_LSHIFT, 0 },
	{ PadItemType_Button, VK_NUMPAD8, 0 },
	{ PadItemType_Button, VK_NUMPAD2, 0 },
	{ PadItemType_Button, VK_NUMPAD4, 0 },
	{ PadItemType_Button, VK_NUMPAD6, 0 },
	true,
};

FullConfig_Pad DefaultControllerConfig = {
	7849,
	8689,
	true,
	false,
	30,
	1.0f,
	false,
	0,
	{ PadItemType_Axis, SDL_CONTROLLER_AXIS_LEFTY, -1 },
	{ PadItemType_Axis, SDL_CONTROLLER_AXIS_LEFTY, 1 },
	{ PadItemType_Axis, SDL_CONTROLLER_AXIS_LEFTX, -1 },
	{ PadItemType_Axis, SDL_CONTROLLER_AXIS_LEFTX, 1 },
	{ PadItemType_Axis, SDL_CONTROLLER_AXIS_RIGHTY, -1 },
	{ PadItemType_Axis, SDL_CONTROLLER_AXIS_RIGHTY, 1 },
	{ PadItemType_Axis, SDL_CONTROLLER_AXIS_RIGHTX, -1 },
	{ PadItemType_Axis, SDL_CONTROLLER_AXIS_RIGHTX, 1 },
	{ PadItemType_Button, SDL_CONTROLLER_BUTTON_START, 0 },
	{ PadItemType_Button, SDL_CONTROLLER_BUTTON_A, 0 },
	{ PadItemType_Button, SDL_CONTROLLER_BUTTON_B, 0 },
	{ PadItemType_Button, SDL_CONTROLLER_BUTTON_X, 0 },
	{ PadItemType_Button, SDL_CONTROLLER_BUTTON_Y, 0 },
	{ PadItemType_Axis, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 1 },
	{ PadItemType_Axis, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 1 },
	{ PadItemType_Button, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 0 },
	{ PadItemType_Button, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, 0 },
	{ PadItemType_Button, SDL_CONTROLLER_BUTTON_RIGHTSTICK, 0 },
	{ PadItemType_Button, SDL_CONTROLLER_BUTTON_LEFTSTICK, 0 },
	{ PadItemType_Button, SDL_CONTROLLER_BUTTON_BACK, 0 },
	{ PadItemType_Button, SDL_CONTROLLER_BUTTON_DPAD_UP, 0 },
	{ PadItemType_Button, SDL_CONTROLLER_BUTTON_DPAD_DOWN, 0 },
	{ PadItemType_Button, SDL_CONTROLLER_BUTTON_DPAD_LEFT, 0 },
	{ PadItemType_Button, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 0 },
	true,
};

static void* RumbleA_ptr            = reinterpret_cast<void*>(0x004BCBC0);
static void* RumbleB_ptr            = reinterpret_cast<void*>(0x004BCC10);
static void* UpdateControllers_ptr  = reinterpret_cast<void*>(0x0040F460);
static void* AnalogHook_ptr         = reinterpret_cast<void*>(0x0040F343);
static void* InitRawControllers_ptr = reinterpret_cast<void*>(0x0040F451); // End of function (hook)

PointerInfo jumps[] = {
	{ rumble::pdVibMxStop, rumble::pdVibMxStop_r },
	{ RumbleA_ptr, rumble::RumbleA_r },
	{ RumbleB_ptr, rumble::RumbleB_r },
	{ AnalogHook_ptr, input::WriteAnalogs_hook },
	{ InitRawControllers_ptr, input::InitRawControllers_hook },
	{ EnableController, input::EnableController_r },
	{ DisableController, input::DisableController_r },
	{ reinterpret_cast<void*>(0x0042D52D), rumble::default_rumble },
	// Used to skip over the standard controller update function.
	// This has no effect on the OnInput hook.
	{ UpdateControllers_ptr, reinterpret_cast<void*>(0x0040FDB3) }
};

static std::string build_mod_path(const char* modpath, const char* path)
{
	std::stringstream result;
	char workingdir[FILENAME_MAX] {};

	result << _getcwd(workingdir, FILENAME_MAX) << "\\" << modpath << "\\" << path;

	return result.str();
}

void CopyBinds(PadConfigItem* destination, PadConfigItem* source)
{
	destination->direction = source->direction;
	destination->index = source->index;
	destination->type = source->type;
}

void ClearBinds(PadConfigItem* item)
{
	item->direction = 0;
	item->type = -1;
	item->index = -1;
}

void ClearControllerBindings(int i)
{
	ClearBinds(&PlayerConfigs[i].s0_up);
	ClearBinds(&PlayerConfigs[i].s0_down);
	ClearBinds(&PlayerConfigs[i].s0_left);
	ClearBinds(&PlayerConfigs[i].s0_right);
	ClearBinds(&PlayerConfigs[i].s1_up);
	ClearBinds(&PlayerConfigs[i].s1_down);
	ClearBinds(&PlayerConfigs[i].s1_left);
	ClearBinds(&PlayerConfigs[i].s1_right);
	ClearBinds(&PlayerConfigs[i].dpad_up);
	ClearBinds(&PlayerConfigs[i].dpad_down);
	ClearBinds(&PlayerConfigs[i].dpad_left);
	ClearBinds(&PlayerConfigs[i].dpad_right);
	ClearBinds(&PlayerConfigs[i].a);
	ClearBinds(&PlayerConfigs[i].b);
	ClearBinds(&PlayerConfigs[i].x);
	ClearBinds(&PlayerConfigs[i].y);
	ClearBinds(&PlayerConfigs[i].z);
	ClearBinds(&PlayerConfigs[i].c);
	ClearBinds(&PlayerConfigs[i].d);
	ClearBinds(&PlayerConfigs[i].e);
	ClearBinds(&PlayerConfigs[i].h);
	ClearBinds(&PlayerConfigs[i].rt);
	ClearBinds(&PlayerConfigs[i].lt);
	ClearBinds(&PlayerConfigs[i].start);
	PlayerConfigs[i].dpad_camera = true;
}

void SetUpDefaultBindings()
{
	//Keyboard
	CopyBinds(&PlayerConfigs[0].s0_up, &DefaultKeyboardConfig.s0_up);
	CopyBinds(&PlayerConfigs[0].s0_down, &DefaultKeyboardConfig.s0_down);
	CopyBinds(&PlayerConfigs[0].s0_left, &DefaultKeyboardConfig.s0_left);
	CopyBinds(&PlayerConfigs[0].s0_right, &DefaultKeyboardConfig.s0_right);
	CopyBinds(&PlayerConfigs[0].s1_up, &DefaultKeyboardConfig.s1_up);
	CopyBinds(&PlayerConfigs[0].s1_down, &DefaultKeyboardConfig.s1_down);
	CopyBinds(&PlayerConfigs[0].s1_left, &DefaultKeyboardConfig.s1_left);
	CopyBinds(&PlayerConfigs[0].s1_right, &DefaultKeyboardConfig.s1_right);
	CopyBinds(&PlayerConfigs[0].dpad_up, &DefaultKeyboardConfig.dpad_up);
	CopyBinds(&PlayerConfigs[0].dpad_down, &DefaultKeyboardConfig.dpad_down);
	CopyBinds(&PlayerConfigs[0].dpad_left, &DefaultKeyboardConfig.dpad_left);
	CopyBinds(&PlayerConfigs[0].dpad_right, &DefaultKeyboardConfig.dpad_right);
	CopyBinds(&PlayerConfigs[0].a, &DefaultKeyboardConfig.a);
	CopyBinds(&PlayerConfigs[0].b, &DefaultKeyboardConfig.b);
	CopyBinds(&PlayerConfigs[0].x, &DefaultKeyboardConfig.x);
	CopyBinds(&PlayerConfigs[0].y, &DefaultKeyboardConfig.y);
	CopyBinds(&PlayerConfigs[0].z, &DefaultKeyboardConfig.z);
	CopyBinds(&PlayerConfigs[0].c, &DefaultKeyboardConfig.c);
	CopyBinds(&PlayerConfigs[0].d, &DefaultKeyboardConfig.d);
	CopyBinds(&PlayerConfigs[0].e, &DefaultKeyboardConfig.e);
	CopyBinds(&PlayerConfigs[0].h, &DefaultKeyboardConfig.h);
	CopyBinds(&PlayerConfigs[0].rt, &DefaultKeyboardConfig.rt);
	CopyBinds(&PlayerConfigs[0].lt, &DefaultKeyboardConfig.lt);
	CopyBinds(&PlayerConfigs[0].start, &DefaultKeyboardConfig.start);
	PlayerConfigs[0].dpad_camera = DefaultKeyboardConfig.dpad_camera;
	//Keyboard secondary
	ClearBinds(&PlayerConfigs[1].s0_up); // &DefaultKeyboardConfig.s0_up);
	ClearBinds(&PlayerConfigs[1].s0_down); // &DefaultKeyboardConfig.s0_down);
	ClearBinds(&PlayerConfigs[1].s0_left); // &DefaultKeyboardConfig.s0_left);
	ClearBinds(&PlayerConfigs[1].s0_right); // &DefaultKeyboardConfig.s0_right);
	ClearBinds(&PlayerConfigs[1].s1_up); // &DefaultKeyboardConfig.s1_up);
	ClearBinds(&PlayerConfigs[1].s1_down); // &DefaultKeyboardConfig.s1_down);
	ClearBinds(&PlayerConfigs[1].s1_left); // &DefaultKeyboardConfig.s1_left);
	ClearBinds(&PlayerConfigs[1].s1_right); // &DefaultKeyboardConfig.s1_right);
	ClearBinds(&PlayerConfigs[1].dpad_up); // &DefaultKeyboardConfig.dpad_up);
	ClearBinds(&PlayerConfigs[1].dpad_down); // &DefaultKeyboardConfig.dpad_down);
	ClearBinds(&PlayerConfigs[1].dpad_left); // &DefaultKeyboardConfig.dpad_left);
	ClearBinds(&PlayerConfigs[1].dpad_right); // &DefaultKeyboardConfig.dpad_right);
	ClearBinds(&PlayerConfigs[1].a); // &DefaultKeyboardConfig.a);
	ClearBinds(&PlayerConfigs[1].b); // &DefaultKeyboardConfig.b);
	ClearBinds(&PlayerConfigs[1].x); // &DefaultKeyboardConfig.x);
	ClearBinds(&PlayerConfigs[1].y); // &DefaultKeyboardConfig.y);
	ClearBinds(&PlayerConfigs[1].z); // &DefaultKeyboardConfig.z);
	ClearBinds(&PlayerConfigs[1].c); // &DefaultKeyboardConfig.c);
	ClearBinds(&PlayerConfigs[1].d); // &DefaultKeyboardConfig.d);
	ClearBinds(&PlayerConfigs[1].e); // &DefaultKeyboardConfig.e);
	ClearBinds(&PlayerConfigs[1].h); // &DefaultKeyboardConfig.h);
	ClearBinds(&PlayerConfigs[1].rt); // &DefaultKeyboardConfig.rt);
	ClearBinds(&PlayerConfigs[1].lt); // &DefaultKeyboardConfig.lt);
	ClearBinds(&PlayerConfigs[1].start); // &DefaultKeyboardConfig.start);
	PlayerConfigs[1].dpad_camera = false;
	//Pad 1 (third device)
	CopyBinds(&PlayerConfigs[2].s0_up, &DefaultControllerConfig.s0_up);
	CopyBinds(&PlayerConfigs[2].s0_down, &DefaultControllerConfig.s0_down);
	CopyBinds(&PlayerConfigs[2].s0_left, &DefaultControllerConfig.s0_left);
	CopyBinds(&PlayerConfigs[2].s0_right, &DefaultControllerConfig.s0_right);
	CopyBinds(&PlayerConfigs[2].s1_up, &DefaultControllerConfig.s1_up);
	CopyBinds(&PlayerConfigs[2].s1_down, &DefaultControllerConfig.s1_down);
	CopyBinds(&PlayerConfigs[2].s1_left, &DefaultControllerConfig.s1_left);
	CopyBinds(&PlayerConfigs[2].s1_right, &DefaultControllerConfig.s1_right);
	CopyBinds(&PlayerConfigs[2].dpad_up, &DefaultControllerConfig.dpad_up);
	CopyBinds(&PlayerConfigs[2].dpad_down, &DefaultControllerConfig.dpad_down);
	CopyBinds(&PlayerConfigs[2].dpad_left, &DefaultControllerConfig.dpad_left);
	CopyBinds(&PlayerConfigs[2].dpad_right, &DefaultControllerConfig.dpad_right);
	CopyBinds(&PlayerConfigs[2].a, &DefaultControllerConfig.a);
	CopyBinds(&PlayerConfigs[2].b, &DefaultControllerConfig.b);
	CopyBinds(&PlayerConfigs[2].x, &DefaultControllerConfig.x);
	CopyBinds(&PlayerConfigs[2].y, &DefaultControllerConfig.y);
	CopyBinds(&PlayerConfigs[2].z, &DefaultControllerConfig.z);
	CopyBinds(&PlayerConfigs[2].c, &DefaultControllerConfig.c);
	CopyBinds(&PlayerConfigs[2].d, &DefaultControllerConfig.d);
	CopyBinds(&PlayerConfigs[2].e, &DefaultControllerConfig.e);
	CopyBinds(&PlayerConfigs[2].h, &DefaultControllerConfig.h);
	CopyBinds(&PlayerConfigs[2].rt, &DefaultControllerConfig.rt);
	CopyBinds(&PlayerConfigs[2].lt, &DefaultControllerConfig.lt);
	CopyBinds(&PlayerConfigs[2].start, &DefaultControllerConfig.start);
	PlayerConfigs[2].dpad_camera = DefaultControllerConfig.dpad_camera;
}

SDL_GameControllerButton TranslateSteamXBOXToSDL_Buttons(int index)
{
	SDL_GameControllerButton result;
	if (index == 0) result = SDL_CONTROLLER_BUTTON_A;
	else if (index == 1) result = SDL_CONTROLLER_BUTTON_B;
	else if (index == 2) result = SDL_CONTROLLER_BUTTON_X;
	else if (index == 3) result = SDL_CONTROLLER_BUTTON_Y;
	else if (index == 5) result = SDL_CONTROLLER_BUTTON_BACK;
	else if (index == 4) result = SDL_CONTROLLER_BUTTON_START;
	else if (index == 6) result = SDL_CONTROLLER_BUTTON_LEFTSTICK;
	else if (index == 7) result = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
	else if (index == 8) result = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
	else if (index == 9) result = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
	return result;
}

SDL_GameControllerButton TranslateSteamDINPUTToSDL_Buttons(int index)
{
	SDL_GameControllerButton result;
	if (index == 0) result = SDL_CONTROLLER_BUTTON_A;
	else if (index == 1) result = SDL_CONTROLLER_BUTTON_B;
	else if (index == 2) result = SDL_CONTROLLER_BUTTON_X;
	else if (index == 3) result = SDL_CONTROLLER_BUTTON_Y;
	else if (index == 5) result = SDL_CONTROLLER_BUTTON_BACK;
	else if (index == 4) result = SDL_CONTROLLER_BUTTON_START;
	else if (index == 6) result = SDL_CONTROLLER_BUTTON_LEFTSTICK;
	else if (index == 7) result = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
	else if (index == 8) result = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
	else if (index == 9) result = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
	else if (index == 10) result = (SDL_GameControllerButton)10;
	else if (index == 11) result = (SDL_GameControllerButton)11;
	else if (index == 12) result = (SDL_GameControllerButton)12;
	return result;
}

void TranslateSteamtoSDL(PadConfigItem *padconfig_sdl, SteamBinding *padconfig_steam, int input_mode)
{
	//Input modes: 0 - Keyboard, 1 - XInput, 2 - DInput
	/*
	LX- type 1 index 0 dir -1
	LX+ type 1 index 0 dir 1
	LY- type 1 index 1 dir -1
	LY+ type 1 index 1 dir 1
	RX- type 1 index 2 dir -1
	RX+ type 1 index 2 dir 1
	RY- type 2 index 2 dir -1
	RY+ type 2 index 2 dir 1
	RT  type 3 index 0 dir 0
	LT  type 3 index 1 dir 0
	*/
	//Quit
	if (padconfig_steam->type == -1)
	{
		return;
	}
	//Axes
	//Axis type 1 (X Y Z)
	if (padconfig_steam->type == 1)
	{
		padconfig_sdl->type = PadItemType_Axis;
		padconfig_sdl->index = padconfig_steam->index;
		padconfig_sdl->direction = padconfig_steam->dir;
		PrintDebug("Axis type 1, index %d, dir %d\n", padconfig_sdl->index, padconfig_sdl->direction);
	}
	//Axis type 2 (RX RY RZ)
	else if (padconfig_steam->type == 2)
	{
		padconfig_sdl->type = PadItemType_Axis;
		padconfig_sdl->index = padconfig_steam->index+3;
		padconfig_sdl->direction = padconfig_steam->dir;
		PrintDebug("Axis type 2, index %d, dir %d\n", padconfig_sdl->index, padconfig_sdl->direction);
	}
	//Axis type 3 (RX RY in XInput)
	else if (padconfig_steam->type == 3)
	{
		padconfig_sdl->type = PadItemType_Axis;
		padconfig_sdl->direction = 1;
		if (padconfig_steam->index == 0) padconfig_sdl->index = 2;
		else if (padconfig_steam->index == 1) padconfig_sdl->index = 5;
		PrintDebug("Axis type 1, index %d, dir %d\n", padconfig_sdl->index, padconfig_sdl->direction);
	}
	//D-Pad
	else if (padconfig_steam->type == 4)
	{
		if (padconfig_steam->dir == 0) padconfig_sdl->index = SDL_CONTROLLER_BUTTON_DPAD_UP;
		if (padconfig_steam->dir == 18000) padconfig_sdl->index = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
		if (padconfig_steam->dir == 27000) padconfig_sdl->index = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
		if (padconfig_steam->dir == 9000) padconfig_sdl->index = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
	}
	//Buttons
	else
	{
		padconfig_sdl->type = PadItemType_Button;
		if (!input_mode) padconfig_sdl->index = TranslateSteamToWindows(padconfig_steam->index);
		//else if (input_mode == 1) padconfig_sdl->index = TranslateSteamXBOXToSDL_Buttons(padconfig_steam->index);
		else// if (input_mode == 2)
		{
			padconfig_sdl->index = TranslateSteamDINPUTToSDL_Buttons(padconfig_steam->index);
			//PrintDebug("Steam %d, SDL %d\n", padconfig_steam->index, padconfig_sdl->index);
		}
		padconfig_sdl->direction = 0;
	}
}

void SetUpFinalBindings(int i)
{
	int mode = 2; //DInput
	if (XMLConfig[i].guid == "11111111-0000-0000-0000-000000000000" || XMLConfig[i].guid == "00000000-0000-0000-0000-000000000000" || XMLConfig[i].guid == "22222222-0000-0000-0000-000000000000")
	{
		mode = 0; //Keyboard
	}
	else if (XMLConfig[i].guid == "11111111-1111-1111-1111-111111111111") mode = 1; //XInput
	PlayerConfigs[i].DeadzoneL = XMLConfig[i].DeadzoneL;
	PlayerConfigs[i].DeadzoneR = XMLConfig[i].DeadzoneR;
	PlayerConfigs[i].RadialL= XMLConfig[i].RadialL;
	PlayerConfigs[i].RadialR = XMLConfig[i].RadialR;
	PlayerConfigs[i].TriggerThreshold = XMLConfig[i].TriggerThreshold;
	PlayerConfigs[i].RumbleFactor = XMLConfig[i].RumbleFactor;
	PlayerConfigs[i].MegaRumble = XMLConfig[i].MegaRumble;
	PlayerConfigs[i].RumbleMinTime = XMLConfig[i].RumbleMinTime;
	TranslateSteamtoSDL(&PlayerConfigs[i].s0_up, &XMLConfig[i].s0_up, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].s0_down, &XMLConfig[i].s0_down, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].s0_left, &XMLConfig[i].s0_left, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].s0_right, &XMLConfig[i].s0_right, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].s1_up, &XMLConfig[i].s1_up, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].s1_down, &XMLConfig[i].s1_down, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].s1_left, &XMLConfig[i].s1_left, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].s1_right, &XMLConfig[i].s1_right, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].start, &XMLConfig[i].start, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].a, &XMLConfig[i].a, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].b, &XMLConfig[i].b, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].x, &XMLConfig[i].x, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].y, &XMLConfig[i].y, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].lt, &XMLConfig[i].lt, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].rt, &XMLConfig[i].rt, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].z, &XMLConfig[i].z, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].c, &XMLConfig[i].c, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].d, &XMLConfig[i].d, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].e, &XMLConfig[i].e, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].h, &XMLConfig[i].h, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].dpad_up, &XMLConfig[i].dpad_up, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].dpad_down, &XMLConfig[i].dpad_down, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].dpad_left, &XMLConfig[i].dpad_left, mode);
	TranslateSteamtoSDL(&PlayerConfigs[i].dpad_right, &XMLConfig[i].dpad_right, mode);
	if (input::debug)
	{
		PrintDebug("[Input] Loading controller %d, GUID %s, mode %d\n", i, XMLConfig[i].guid.c_str(), mode);
		PrintDebug("	Deadzones: L %d, R %d, T %d\n", PlayerConfigs[i].DeadzoneL, PlayerConfigs[i].DeadzoneR, PlayerConfigs[i].TriggerThreshold);
		PrintDebug("	Rumble: factor %f, mega %d, mintime %d\n", PlayerConfigs[i].RumbleFactor, PlayerConfigs[i].MegaRumble, PlayerConfigs[i].RumbleMinTime);
		PrintDebug("	S0 UP   : %d, %d, %d\n", PlayerConfigs[i].s0_up.type, PlayerConfigs[i].s0_up.index, PlayerConfigs[i].s0_up.direction);
		PrintDebug("	S0 DOWN : %d, %d, %d\n", PlayerConfigs[i].s0_down.type, PlayerConfigs[i].s0_down.index, PlayerConfigs[i].s0_down.direction);
		PrintDebug("	S0 LEFT : %d, %d, %d\n", PlayerConfigs[i].s0_left.type, PlayerConfigs[i].s0_left.index, PlayerConfigs[i].s0_left.direction);
		PrintDebug("	S0 RIGHT : %d, %d, %d\n", PlayerConfigs[i].s0_right.type, PlayerConfigs[i].s0_right.index, PlayerConfigs[i].s0_right.direction);
		PrintDebug("	S1 UP   : %d, %d, %d\n", PlayerConfigs[i].s1_up.type, PlayerConfigs[i].s1_up.index, PlayerConfigs[i].s1_up.direction);
		PrintDebug("	S1 DOWN : %d, %d, %d\n", PlayerConfigs[i].s1_down.type, PlayerConfigs[i].s1_down.index, PlayerConfigs[i].s1_down.direction);
		PrintDebug("	S1 LEFT : %d, %d, %d\n", PlayerConfigs[i].s1_left.type, PlayerConfigs[i].s1_left.index, PlayerConfigs[i].s1_left.direction);
		PrintDebug("	S1 RIGHT : %d, %d, %d\n", PlayerConfigs[i].s1_right.type, PlayerConfigs[i].s1_right.index, PlayerConfigs[i].s1_right.direction);
		PrintDebug("	A        : %d, %d, %d\n", PlayerConfigs[i].a.type, PlayerConfigs[i].a.index, PlayerConfigs[i].a.direction);
		PrintDebug("	B        : %d, %d, %d\n", PlayerConfigs[i].b.type, PlayerConfigs[i].b.index, PlayerConfigs[i].b.direction);
		PrintDebug("	X        : %d, %d, %d\n", PlayerConfigs[i].x.type, PlayerConfigs[i].x.index, PlayerConfigs[i].x.direction);
		PrintDebug("	Y        : %d, %d, %d\n", PlayerConfigs[i].y.type, PlayerConfigs[i].y.index, PlayerConfigs[i].y.direction);
		PrintDebug("	Z        : %d, %d, %d\n", PlayerConfigs[i].z.type, PlayerConfigs[i].z.index, PlayerConfigs[i].z.direction);
		PrintDebug("	C        : %d, %d, %d\n", PlayerConfigs[i].c.type, PlayerConfigs[i].c.index, PlayerConfigs[i].c.direction);
		PrintDebug("	D        : %d, %d, %d\n", PlayerConfigs[i].d.type, PlayerConfigs[i].d.index, PlayerConfigs[i].d.direction);
		PrintDebug("	E        : %d, %d, %d\n", PlayerConfigs[i].e.type, PlayerConfigs[i].e.index, PlayerConfigs[i].e.direction);
		PrintDebug("	H        : %d, %d, %d\n", PlayerConfigs[i].h.type, PlayerConfigs[i].h.index, PlayerConfigs[i].h.direction);
		PrintDebug("	DP UP   : %d, %d, %d\n", PlayerConfigs[i].dpad_up.type, PlayerConfigs[i].dpad_up.index, PlayerConfigs[i].dpad_up.direction);
		PrintDebug("	DP DOWN : %d, %d, %d\n", PlayerConfigs[i].dpad_down.type, PlayerConfigs[i].dpad_down.index, PlayerConfigs[i].dpad_down.direction);
		PrintDebug("	DP LEFT : %d, %d, %d\n", PlayerConfigs[i].dpad_left.type, PlayerConfigs[i].dpad_left.index, PlayerConfigs[i].dpad_left.direction);
		PrintDebug("	DP RIGHT : %d, %d, %d\n", PlayerConfigs[i].dpad_right.type, PlayerConfigs[i].dpad_right.index, PlayerConfigs[i].dpad_right.direction);
	}
}

int GetConfigBinding(TiXmlElement *pElem, TiXmlHandle *hRoot, const char* section, const char* item, const char* attribute)
{
	int temp = 0;
	pElem = hRoot->FirstChild(section).FirstChild(item).Element();
	if (!pElem) return -1;
	pElem->QueryIntAttribute(attribute, &temp);
	return temp;
}

int GetPadAttribute_Int(TiXmlElement* root, const char* attribute, int default, int min, int max)
{
	int result=0;
	if (!root->Attribute(attribute)) result = default;
	else root->QueryIntAttribute(attribute, &result);
	if (result > max) result = max;
	if (result < min) result = min;
	return result;
}

float GetPadAttribute_Float(TiXmlElement* root, const char* attribute, float default, float min, float max)
{
	float result;
	if (!root->Attribute(attribute)) result = default;
	else root->QueryFloatAttribute(attribute, &result);
	if (result > max) result = max;
	if (result < min) result = min;
	return result;
}

bool GetPadAttribute_Bool(TiXmlElement* root, const char* attribute, bool default)
{
	bool result;
	if (!root->Attribute(attribute)) result = default;
	else root->QueryBoolAttribute(attribute, &result);
	return result;
}

std::string GetButtonName(SteamBinding bind)
{
	/*
	XML axis types:
	1 index 0 - X; (SDL a0)
	1 index 1 - Y; (SDL a1)
	1 index 2 - Z; (SDL a2) sometimes divided for a2+ and a2-
	2 index 0 - XR (SDL a3) fails in launcher
	2 index 1 - YR (SDL a4) fails in launcher
	2 index 2 - ZR (SDL a5 or a4 if the previous axis doesn't exist)
	XInput only:
	3 index 0 - LT (SDL a2)
	3 index 1 - RT (SDL a5)
	*/
	char dpad_dir;
	char buff[100];
	std::string result;
	//Button
	if (bind.type == 5)
	{
		snprintf(buff, sizeof(buff), "b%d", bind.index);
	}
	//D-Pad
	else if (bind.type == 4)
	{
		if (bind.dir == 0) dpad_dir = 1; //up
		else if (bind.dir == 18000) dpad_dir = 4; //down
		else if (bind.dir == 27000) dpad_dir = 8; //left
		else if (bind.dir == 9000) dpad_dir = 2; //right
		sprintf_s(buff, "h%d.%d", bind.index, dpad_dir);
	}
	//Triggers (XInput)
	else if (bind.type == 3)
	{
		if (bind.index == 0) sprintf_s(buff, "a2");
		else if (bind.index == 1) sprintf_s(buff, "a5");
	}
	//Axes
	else
	{
		//Axis type 1 (X Y Z)
		if (bind.type == 1)
		{
			if (bind.dir == -1) sprintf_s(buff, "-a%d", bind.index);
			else sprintf_s(buff, "+a%d", bind.index);
		}
		//Axis type 2 (RX RY RZ)
		else if (bind.type == 2)
		{

			if (bind.dir == -1) sprintf_s(buff, "-a%d", bind.index + 3);
			else sprintf_s(buff, "+a%d", bind.index + 3);
		}
	}
	result = buff;
	return result;
}

void GenerateSDLBinds(int i)
{
	/*
	SDL mappings:
	SDL_CONTROLLER_BUTTON_BACK
	SDL_CONTROLLER_BUTTON_GUIDE 
	E button/R3: SDL_CONTROLLER_BUTTON_LEFTSTICK
	D button/L3: SDL_CONTROLLER_BUTTON_RIGHTSTICK 
	C button/R2: SDL_CONTROLLER_BUTTON_LEFTSHOULDER
	Z button/L2: SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
	*/
	char mappingstring[800];
	std::string ControllerName = "ASS";
	sprintf_s(mappingstring, "%s,%s,a:%s,b:%s,y:%s,x:%s,start:%s,dpup:%s,dpleft:%s,dpdown:%s,dpright:%s,leftshoulder:%s,rightshoulder:%s,leftstick:%s,rightstick:%s,-leftx:%s,+leftx:%s,-lefty:%s,+lefty:%s,-rightx:%s,+rightx:%s,-righty:%s,+righty:%s,lefttrigger:%s,righttrigger:%s,platform:Windows,", 
		XMLConfig[i].guid.c_str(), 
		ControllerName.c_str(), 
		GetButtonName(XMLConfig[i].a).c_str(),
		GetButtonName(XMLConfig[i].b).c_str(),
		GetButtonName(XMLConfig[i].y).c_str(),
		GetButtonName(XMLConfig[i].x).c_str(),
		GetButtonName(XMLConfig[i].start).c_str(),
		GetButtonName(XMLConfig[i].dpad_up).c_str(),
		GetButtonName(XMLConfig[i].dpad_left).c_str(),
		GetButtonName(XMLConfig[i].dpad_down).c_str(),
		GetButtonName(XMLConfig[i].dpad_right).c_str(),
		GetButtonName(XMLConfig[i].z).c_str(),
		GetButtonName(XMLConfig[i].c).c_str(),
		GetButtonName(XMLConfig[i].e).c_str(),
		GetButtonName(XMLConfig[i].d).c_str(),
		GetButtonName(XMLConfig[i].s0_left).c_str(),
		GetButtonName(XMLConfig[i].s0_right).c_str(),
		GetButtonName(XMLConfig[i].s0_up).c_str(),
		GetButtonName(XMLConfig[i].s0_down).c_str(),
		GetButtonName(XMLConfig[i].s1_left).c_str(),
		GetButtonName(XMLConfig[i].s1_right).c_str(),
		GetButtonName(XMLConfig[i].s1_up).c_str(),
		GetButtonName(XMLConfig[i].s1_down).c_str(),
		GetButtonName(XMLConfig[i].lt).c_str(),
		GetButtonName(XMLConfig[i].rt).c_str()
	);
	//This string shows an example of a valid mapping for a controller
	/*
	"03000000341a00003608000000000000,PS3 Controller,a:b1,b:b2,y:b3,x:b0,start:b9,guide:b12,back:b8,dpup:h0.1,dpleft:h0.8,dpdown:h0.4,dpright:h0.2,leftshoulder:b4,rightshoulder:b5,leftstick:b10,rightstick:b11,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:b6,righttrigger:b7"
	*/
	//PrintDebug("SDL string for controller %d:\n", i);
	//PrintDebug("%s\n", mappingstring);
}

void LoadXMLBinds(int i, TiXmlElement* root, TiXmlElement* pElem, TiXmlHandle* hRoot)
{
	//General
	XMLConfig[i].guid = root->Attribute("guid");
	XMLConfig[i].DeadzoneL = GetPadAttribute_Int(root, "DeadzoneL", 7849, 0, 32767);
	XMLConfig[i].DeadzoneR = GetPadAttribute_Int(root, "DeadzoneR", 8689, 0, 32767);
	XMLConfig[i].RadialL = GetPadAttribute_Bool(root, "RadialL", true);
	XMLConfig[i].RadialR = GetPadAttribute_Bool(root, "RadialR", true);
	XMLConfig[i].TriggerThreshold = GetPadAttribute_Int(root, "TriggerThreshold", 30, 0, 32767);
	XMLConfig[i].RumbleFactor = GetPadAttribute_Float(root, "RumbleFactor", 1.0f, 0.0f, 1.0f);
	XMLConfig[i].MegaRumble = GetPadAttribute_Bool(root, "MegaRumble", false);
	XMLConfig[i].RumbleMinTime = GetPadAttribute_Int(root, "RumbleMinTime", 0, 0, 32767);
	//Start
	XMLConfig[i].start.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_START", "type");
	XMLConfig[i].start.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_START", "index");
	XMLConfig[i].start.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_START", "dir");
	//Up
	XMLConfig[i].s0_up.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_UP", "type");
	XMLConfig[i].s0_up.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_UP", "index");
	XMLConfig[i].s0_up.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_UP", "dir");
	//Down
	XMLConfig[i].s0_down.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_DOWN", "type");
	XMLConfig[i].s0_down.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_DOWN", "index");
	XMLConfig[i].s0_down.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_DOWN", "dir");
	//Left
	XMLConfig[i].s0_left.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_LEFT", "type");
	XMLConfig[i].s0_left.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_LEFT", "index");
	XMLConfig[i].s0_left.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_LEFT", "dir");
	//Right
	XMLConfig[i].s0_right.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_RIGHT", "type");
	XMLConfig[i].s0_right.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_RIGHT", "index");
	XMLConfig[i].s0_right.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_RIGHT", "dir");
	//A
	XMLConfig[i].a.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_A", "type");
	XMLConfig[i].a.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_A", "index");
	XMLConfig[i].a.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_A", "dir");
	//B
	XMLConfig[i].b.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_B", "type");
	XMLConfig[i].b.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_B", "index");
	XMLConfig[i].b.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_B", "dir");
	//X
	XMLConfig[i].x.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_X", "type");
	XMLConfig[i].x.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_X", "index");
	XMLConfig[i].x.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_X", "dir");
	//Y
	XMLConfig[i].y.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_Y", "type");
	XMLConfig[i].y.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_Y", "index");
	XMLConfig[i].y.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_Y", "dir");
	//LS
	XMLConfig[i].lt.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_LS", "type");
	XMLConfig[i].lt.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_LS", "index");
	XMLConfig[i].lt.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_LS", "dir");
	//RS
	XMLConfig[i].rt.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_RS", "type");
	XMLConfig[i].rt.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_RS", "index");
	XMLConfig[i].rt.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_RS", "dir");
	//Z
	XMLConfig[i].z.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_L2", "type");
	XMLConfig[i].z.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_L2", "index");
	XMLConfig[i].z.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_L2", "dir");
	//C
	XMLConfig[i].c.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R2", "type");
	XMLConfig[i].c.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R2", "index");
	XMLConfig[i].c.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R2", "dir");
	//D
	XMLConfig[i].d.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_L3", "type");
	XMLConfig[i].d.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_L3", "index");
	XMLConfig[i].d.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_L3", "dir");
	//E
	XMLConfig[i].e.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R3", "type");
	XMLConfig[i].e.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R3", "index");
	XMLConfig[i].e.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R3", "dir");
	//H
	XMLConfig[i].h.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_H", "type");
	XMLConfig[i].h.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_H", "index");
	XMLConfig[i].h.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_H", "dir");
	//Up2
	XMLConfig[i].s1_up.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R_UP", "type");
	XMLConfig[i].s1_up.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R_UP", "index");
	XMLConfig[i].s1_up.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R_UP", "dir");
	//Down2
	XMLConfig[i].s1_down.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R_DOWN", "type");
	XMLConfig[i].s1_down.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R_DOWN", "index");
	XMLConfig[i].s1_down.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R_DOWN", "dir");
	//Left2
	XMLConfig[i].s1_left.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R_LEFT", "type");
	XMLConfig[i].s1_left.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R_LEFT", "index");
	XMLConfig[i].s1_left.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R_LEFT", "dir");
	//Right2
	XMLConfig[i].s1_right.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R_RIGHT", "type");
	XMLConfig[i].s1_right.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R_RIGHT", "index");
	XMLConfig[i].s1_right.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_R_RIGHT", "dir");
	//Up d-pad
	XMLConfig[i].dpad_up.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_D_UP", "type");
	XMLConfig[i].dpad_up.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_D_UP", "index");
	XMLConfig[i].dpad_up.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_D_UP", "dir");
	//Down d-pad
	XMLConfig[i].dpad_down.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_D_DOWN", "type");
	XMLConfig[i].dpad_down.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_D_DOWN", "index");
	XMLConfig[i].dpad_down.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_D_DOWN", "dir");
	//Left d-pad
	XMLConfig[i].dpad_left.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_D_LEFT", "type");
	XMLConfig[i].dpad_left.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_D_LEFT", "index");
	XMLConfig[i].dpad_left.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_D_LEFT", "dir");
	//Right d-pad
	XMLConfig[i].dpad_right.type = GetConfigBinding(pElem, hRoot, "player_0", "BTN_D_RIGHT", "type");
	XMLConfig[i].dpad_right.index = GetConfigBinding(pElem, hRoot, "player_0", "BTN_D_RIGHT", "index");
	XMLConfig[i].dpad_right.dir = GetConfigBinding(pElem, hRoot, "player_0", "BTN_D_RIGHT", "dir");
}

void LoadXML(const char* pFilename)
{
	if (input::debug) PrintDebug("[Input] Loading XML file %s\n", pFilename);
	TiXmlDocument doc(pFilename);
	if (!doc.LoadFile()) return;
	TiXmlHandle hDoc(&doc);
	TiXmlElement* pElem;
	TiXmlHandle hRoot(0);
	TiXmlElement* root;
	for (int i = 0; i < 10; i++)
	{
		pElem = hDoc.ChildElement(i).Element();
		if (!pElem) return;
		hRoot = TiXmlHandle(pElem);
		root = hRoot.Element();
		LoadXMLBinds(i, root, pElem, &hRoot);
		GenerateSDLBinds(i);
		ClearControllerBindings(i);
		SetUpFinalBindings(i);
	}
}

extern "C"
{
	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer, nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0 };
	__declspec(dllexport) PointerList Jumps[] = { { arrayptrandlengthT(jumps, int) } };

	__declspec(dllexport) void OnInput()
	{
		input::poll_controllers();
	}

	__declspec(dllexport) void Init(const char* path, const HelperFunctions& helperFunctions)
	{
		WriteCall((void*)0x437547, GetEKey);
		WriteData((__int16**)0x42F0A8, &Demo_Frame);
		WriteData((__int16**)0x42F215, &Demo_Frame);
		WriteData((__int16**)0x42F229, &Demo_Frame);
		SetUpDefaultBindings();
		std::string dll = build_mod_path(path, "SDL2.dll");
		const auto handle = LoadLibraryA(dll.c_str());

		if (handle == nullptr)
		{
			PrintDebug("[Input] Unable to load SDL2.dll.\n");

			MessageBoxA(nullptr, "Error loading SDL. See debug message for details.",
			            "SDL Load Error", MB_OK | MB_ICONERROR);

			return;
		}

		int init;
		if ((init = SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_EVENTS)) != 0)
		{
			PrintDebug("[Input] Unable to initialize SDL. Error code: %i\n", init);
			MessageBoxA(nullptr, "Error initializing SDL. See debug message for details.",
			            "SDL Init Error", MB_OK | MB_ICONERROR);
			return;
		}

		// Disable call to CreateKeyboardDevice
		WriteData<5>(reinterpret_cast<void*>(0x0077F0D7), 0x90i8);
		// Disable call to CreateMouseDevice
		WriteData<5>(reinterpret_cast<void*>(0x0077F03E), 0x90i8);
		// Disable call to DirectInput_Init
		WriteData<5>(reinterpret_cast<void*>(0x0077F205), 0x90i8);

		// EnableControl
		WriteData(reinterpret_cast<bool**>(0x40EF80), &input::controller_enabled[0]);
		WriteData(reinterpret_cast<bool**>(0x40EF86), &input::controller_enabled[1]);
		WriteData(reinterpret_cast<bool**>(0x40EF90), input::controller_enabled);

		// DisableControl
		WriteData(reinterpret_cast<bool**>(0x40EFB0), &input::controller_enabled[0]);
		WriteData(reinterpret_cast<bool**>(0x40EFB6), &input::controller_enabled[1]);
		WriteData(reinterpret_cast<bool**>(0x40EFC0), input::controller_enabled);

		// IsControllerEnabled
		WriteData(reinterpret_cast<bool**>(0x40EFD8), input::controller_enabled);

		// Control
		WriteData(reinterpret_cast<bool**>(0x40FE0D), input::controller_enabled);
		WriteData(reinterpret_cast<bool**>(0x40FE2F), &input::controller_enabled[1]);

		// WriteAnalogs
		WriteData(reinterpret_cast<bool**>(0x40F30C), input::controller_enabled);

		input::controller_enabled[0] = true;
		input::controller_enabled[1] = true;

		std::string dbpath = build_mod_path(path, "gamecontrollerdb.txt");

		if (FileExists(dbpath))
		{
			int result = SDL_GameControllerAddMappingsFromFile(dbpath.c_str());

			if (result == -1)
			{
				PrintDebug("[Input] Error loading gamecontrollerdb: %s\n", SDL_GetError());
			}
			else
			{
				PrintDebug("[Input] Controller mappings loaded: %i\n", result);
			}
		}

		const std::string config_path = build_mod_path(path, "config.ini");

	#ifdef _DEBUG
		const bool debug_default = true;
	#else
		const bool debug_default = false;
	#endif

		IniFile config(config_path);

		input::debug = config.getBool("Config", "Debug", debug_default);

		input::mouse_disabled = config.getBool("Config", "DisableMouse", true);

		if (FileExists("input_config.xml")) LoadXML("input_config.xml");

		// This defaults RadialR to enabled if smooth-cam is detected.
		const bool smooth_cam = GetModuleHandleA("smooth-cam.dll") != nullptr;
		char KeyboardPlayer = max(0, min(7, config.getInt("Config", "KeyboardPlayer", 0)));
		for (ushort i = 0; i < GAMEPAD_COUNT; i++)
		{
			DreamPad::Settings& settings = DreamPad::controllers[i].settings;

			const int deadzone_l = PlayerConfigs[i + 1].DeadzoneL;
			const int deadzone_r = PlayerConfigs[i + 1].DeadzoneR;
			
			settings.set_deadzone_l(deadzone_l);
			settings.set_deadzone_r(deadzone_r);

			settings.radial_l = PlayerConfigs[i + 1].RadialL;
			settings.radial_r = PlayerConfigs[i + 1].RadialR;

			settings.trigger_threshold = PlayerConfigs[i + 1].TriggerThreshold;

			settings.rumble_factor = clamp(PlayerConfigs[i + 1].RumbleFactor, 0.0f, 1.0f);

			settings.mega_rumble = PlayerConfigs[i + 1].MegaRumble;
			settings.rumble_min_time = static_cast<ushort>(PlayerConfigs[i + 1].RumbleMinTime);

			if (i == KeyboardPlayer) settings.allow_keyboard = true; else settings.allow_keyboard = false;

		}

		PrintDebug("[Input] Initialization complete.\n");
	}

	__declspec(dllexport) void OnExit()
	{
		for (auto& i : DreamPad::controllers)
		{
			i.close();
		}
	}
}
