#include "stdafx.h"
#include "SDL.h"
#include <limits.h>
#include <Windows.h>
#include "minmax.h"

#include "DreamPad.h"

static const Uint32 PAD_SUPPORT = 
	PDD_DEV_SUPPORT_TA | PDD_DEV_SUPPORT_TB | PDD_DEV_SUPPORT_TX | PDD_DEV_SUPPORT_TY | PDD_DEV_SUPPORT_ST
#ifdef EXTENDED_BUTTONS
	| PDD_DEV_SUPPORT_TC | PDD_DEV_SUPPORT_TD | PDD_DEV_SUPPORT_TZ
#endif
	| PDD_DEV_SUPPORT_AR | PDD_DEV_SUPPORT_AL
	| PDD_DEV_SUPPORT_KU | PDD_DEV_SUPPORT_KD | PDD_DEV_SUPPORT_KL | PDD_DEV_SUPPORT_KR
	| PDD_DEV_SUPPORT_AX1 | PDD_DEV_SUPPORT_AY1 | PDD_DEV_SUPPORT_AX2 | PDD_DEV_SUPPORT_AY2;

DreamPad DreamPad::Controllers[GAMEPAD_COUNT];

DreamPad::DreamPad() : controller_id(-1), gamepad(nullptr), haptic(nullptr), effect({}), effect_id(-1),
	rumble_state(Motor::None), pad({}), normalized_L(0.0f), normalized_R(0.0f)
{
	// TODO: Properly detect supported rumble types
	effect.leftright.type = SDL_HAPTIC_LEFTRIGHT;
}
DreamPad::~DreamPad()
{
	Close();
}

bool DreamPad::Open(int id)
{
	if (connected)
	{
		Close();
	}

	gamepad = SDL_GameControllerOpen(id);

	if (gamepad == nullptr)
		return connected = false;

	pad.Support = PAD_SUPPORT;

	SDL_Joystick* joystick = SDL_GameControllerGetJoystick(gamepad);

	if (joystick == nullptr)
	{
		return connected = false;
	}

	controller_id = id;
	haptic = SDL_HapticOpenFromJoystick(joystick);

	if (haptic == nullptr)
	{
		return connected = true;
	}

	if (SDL_HapticRumbleSupported(haptic))
	{
		effect_id = SDL_HapticNewEffect(haptic, &effect);
	}
	else
	{
		SDL_HapticClose(haptic);
		haptic = nullptr;
	}

	return connected = true;
}

void DreamPad::Close()
{
	if (!connected)
	{
		return;
	}

	if (haptic != nullptr)
	{
		SDL_HapticDestroyEffect(haptic, effect_id);
		SDL_HapticClose(haptic);

		effect_id = -1;
		haptic = nullptr;
	}

	if (gamepad != nullptr)
	{
		SDL_GameControllerClose(gamepad);
		gamepad = nullptr;
	}

	controller_id = -1;
	connected = false;
}

void DreamPad::Poll(Uint32 add_buttons, const NJS_POINT2I* add_left, const NJS_POINT2I* add_right)
{
	// HACK: 
	if (!connected && add_left == nullptr && add_right == nullptr)
	{
		return;
	}

	SDL_GameControllerUpdate();

	NJS_POINT2I axis;

	axis.x = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTX);
	axis.y = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTY);

	if (add_left != nullptr && (add_left->x != 0 || add_left->y != 0))
	{
		normalized_L = ConvertAxes((NJS_POINT2I*)&pad.LeftStickX, *add_left, settings.deadzoneL, settings.radialL);
	}
	else
	{
		normalized_L = ConvertAxes((NJS_POINT2I*)&pad.LeftStickX, axis, settings.deadzoneL, settings.radialL);
	}

	axis.x = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTX);
	axis.y = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTY);


	if (add_right != nullptr && (add_right->x != 0 || add_right->y != 0))
	{
		normalized_R = ConvertAxes((NJS_POINT2I*)&pad.RightStickX, *add_right, settings.deadzoneR, settings.radialR);
	}
	else
	{
		normalized_R = ConvertAxes((NJS_POINT2I*)&pad.RightStickX, axis, settings.deadzoneR, settings.radialR);
	}

	short lt = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
	short rt = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

	pad.LTriggerPressure = (short)(255.0f * ((float)lt / (float)SHRT_MAX));
	pad.RTriggerPressure = (short)(255.0f * ((float)rt / (float)SHRT_MAX));;

	Uint32 buttons = 0;

	buttons |= DigitalTrigger(pad.LTriggerPressure, settings.triggerThreshold, Buttons_L);
	buttons |= DigitalTrigger(pad.RTriggerPressure, settings.triggerThreshold, Buttons_R);

	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_A))
	{
		buttons |= Buttons_A;
	}
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_B))
	{
		buttons |= Buttons_B;
	}
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_X))
	{
		buttons |= Buttons_X;
	}
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_Y))
	{
		buttons |= Buttons_Y;
	}

	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_START))
	{
		buttons |= Buttons_Start;
	}

#ifdef EXTENDED_BUTTONS
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
	{
		buttons |= Buttons_C;
	}
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_BACK))
	{
		buttons |= Buttons_D;
	}
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
	{
		buttons |= Buttons_Z;
	}
#endif

	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_UP))
	{
		buttons |= Buttons_Up;
	}
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
	{
		buttons |= Buttons_Down;
	}
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
	{
		buttons |= Buttons_Left;
	}
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
	{
		buttons |= Buttons_Right;
	}

	buttons |= add_buttons;

	pad.HeldButtons     = buttons;
	pad.NotHeldButtons  = ~buttons;
	pad.ReleasedButtons = pad.Old & (buttons ^ pad.Old);
	pad.PressedButtons  = buttons & (buttons ^ pad.Old);
	pad.Old             = pad.HeldButtons;
}

void DreamPad::SetActiveMotor(Motor motor, bool enable)
{
	if (effect_id == -1 || haptic == nullptr)
	{
		return;
	}

	if (settings.megaRumble)
	{
		motor = Motor::Both;
	}

	const float f = settings.rumbleFactor;

	if (motor & Motor::Large)
	{
		effect.leftright.large_magnitude = enable ? (ushort)(USHRT_MAX * f) : 0;
		rumble_state = (Motor)(enable ? rumble_state | motor : rumble_state & ~Motor::Large);
	}

	if (motor & Motor::Small)
	{
		effect.leftright.small_magnitude = enable ? (ushort)(USHRT_MAX * f) : 0;
		rumble_state = (Motor)(enable ? rumble_state | motor : rumble_state & ~Motor::Small);
	}

	SDL_HapticUpdateEffect(haptic, effect_id, &effect);
	SDL_HapticRunEffect(haptic, effect_id, 1);
}

void DreamPad::Copy(ControllerData& dest) const
{
	dest = pad;
}

inline int DreamPad::DigitalTrigger(ushort trigger, ushort threshold, int button)
{
	return (trigger > threshold) ? button : 0;
}

float DreamPad::ConvertAxes(NJS_POINT2I* dest, const NJS_POINT2I& source, short deadzone, bool radial) const
{
	if (abs(source.x) < deadzone && abs(source.y) < deadzone)
	{
		*dest = {};
		return 0.0f;
	}

	// This is being intentionally limited to -32767 instead of -32768
	const float x = (float)clamp(source.x, (short)-SHRT_MAX, (short)SHRT_MAX);
	const float y = (float)-clamp(source.y, (short)-SHRT_MAX, (short)SHRT_MAX);

	const float m = sqrt(x * x + y * y);

	const float nx	= (m < deadzone) ? 0 : x / m;	// Normalized (X)
	const float ny	= (m < deadzone) ? 0 : y / m;	// Normalized (Y)
	const float n	= (min((float)SHRT_MAX, m) - deadzone) / ((float)SHRT_MAX - deadzone);

	// In my testing, multiplying -128 to 128 results in 127 instead, which is the desired value.
	dest->x = (radial || abs(source.x) >= deadzone) ? (short)clamp((short)(128 * (nx * n)), (short)-127, (short)127) : 0;
	dest->y = (radial || abs(source.y) >= deadzone) ? (short)clamp((short)(-128 * (ny * n)), (short)-127, (short)127) : 0;

	return n;
}

DreamPad::Settings::Settings()
{
	deadzoneL        = GAMEPAD_LEFT_THUMB_DEADZONE;
	deadzoneR        = GAMEPAD_RIGHT_THUMB_DEADZONE;
	triggerThreshold = GAMEPAD_TRIGGER_THRESHOLD;
	radialL          = true;
	radialR          = false;
	rumbleFactor     = 1.0f;
	megaRumble       = false;
	rumbleMinTime    = 0;
}
void DreamPad::Settings::apply(short deadzoneL, short deadzoneR, bool radialL, bool radialR, uint8 triggerThreshold,
	float rumbleFactor, bool megaRumble, ushort rumbleMinTime)
{
	this->deadzoneL        = clamp(deadzoneL, (short)0, (short)SHRT_MAX);
	this->deadzoneR        = clamp(deadzoneR, (short)0, (short)SHRT_MAX);
	this->radialL          = radialL;
	this->radialR          = radialR;
	this->triggerThreshold = triggerThreshold;
	this->rumbleFactor     = rumbleFactor;
	this->megaRumble       = megaRumble;
	this->rumbleMinTime    = rumbleMinTime;
}
