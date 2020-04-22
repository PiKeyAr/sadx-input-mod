#pragma once

enum SteamInputTypes
{
	InputType_AnalogStick = 1,
	InputType_AnalogStick2 = 2,
	InputType_Trigger = 3,
	InputType_Button = 5,
};

struct SteamBinding
{
	int type;
	int index;
	int dir;
};

struct XMLBindings
{
	std::string guid;
	Uint32 DeadzoneL;
	Uint32 DeadzoneR;
	bool RadialL;
	bool RadialR;
	int TriggerThreshold;
	float RumbleFactor;
	bool MegaRumble;
	int RumbleMinTime;
	SteamBinding s0_up;
	SteamBinding s0_down;
	SteamBinding s0_left;
	SteamBinding s0_right;
	SteamBinding s1_up;
	SteamBinding s1_down;
	SteamBinding s1_left;
	SteamBinding s1_right;
	SteamBinding start;
	SteamBinding a;
	SteamBinding b;
	SteamBinding x;
	SteamBinding y;
	SteamBinding lt;
	SteamBinding rt;
	SteamBinding z;
	SteamBinding c;
	SteamBinding d;
	SteamBinding e;
	SteamBinding h;
	SteamBinding dpad_up;
	SteamBinding dpad_down;
	SteamBinding dpad_left;
	SteamBinding dpad_right;
};