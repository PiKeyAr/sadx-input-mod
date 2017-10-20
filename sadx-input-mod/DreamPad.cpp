#include "stdafx.h"
#include "SDL.h"
#include <limits>
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

DreamPad DreamPad::controllers[GAMEPAD_COUNT];

DreamPad::DreamPad() : controller_id_(-1), gamepad(nullptr), haptic(nullptr), effect({}), effect_id(-1),
	rumble_state(Motor::none), pad({}), normalized_l_(0.0f), normalized_r_(0.0f)
{
	// TODO: Properly detect supported rumble types
	effect.leftright.type = SDL_HAPTIC_LEFTRIGHT;
}
DreamPad::~DreamPad()
{
	close();
}

bool DreamPad::open(int id)
{
	if (connected_)
	{
		close();
	}

	gamepad = SDL_GameControllerOpen(id);

	if (gamepad == nullptr)
	{
		connected_ = false;
		return false;
	}

	pad.Support = PAD_SUPPORT;

	SDL_Joystick* joystick = SDL_GameControllerGetJoystick(gamepad);

	if (joystick == nullptr)
	{
		connected_ = false;
		return false;
	}

	controller_id_ = id;
	haptic = SDL_HapticOpenFromJoystick(joystick);

	if (haptic == nullptr)
	{
		connected_ = true;
		return true;
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

	connected_ = true;
	return true;
}

void DreamPad::close()
{
	if (!connected_)
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

	controller_id_ = -1;
	connected_ = false;
}

void DreamPad::poll()
{
	if (!connected_ && !settings.allow_keyboard)
	{
		return;
	}

	if (connected_)
	{
		SDL_GameControllerUpdate();
	}

	// TODO: keyboard/mouse toggle
	auto& kb = keyboard.dreamcast_data();
	bool allow_keyboard = settings.allow_keyboard;

	if (!connected_ || allow_keyboard && (kb.LeftStickX || kb.LeftStickY))
	{
		normalized_l_ = keyboard.normalized_l();
		pad.LeftStickX = kb.LeftStickX;
		pad.LeftStickY = kb.LeftStickY;
	}
	else
	{
		NJS_POINT2I axis = {
			axis.x = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTX),
			axis.y = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTY)
		};

		normalized_l_ = convert_axes(reinterpret_cast<NJS_POINT2I*>(&pad.LeftStickX), axis, settings.deadzone_l, settings.radial_l);
	}

	if (!connected_ || allow_keyboard && (kb.RightStickX || kb.RightStickY))
	{
		normalized_r_ = keyboard.normalized_r();
		pad.RightStickX = kb.RightStickX;
		pad.RightStickY = kb.RightStickY;
	}
	else
	{
		NJS_POINT2I axis = {
			axis.x = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTX),
			axis.y = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTY)
		};

		normalized_r_ = convert_axes(reinterpret_cast<NJS_POINT2I*>(&pad.RightStickX), axis, settings.deadzone_r, settings.radial_r);
	}

	constexpr short short_max = std::numeric_limits<short>::max();

	if (!connected_ || allow_keyboard && kb.LTriggerPressure)
	{
		pad.LTriggerPressure = kb.LTriggerPressure;
	}
	else
	{
		auto lt = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
		pad.LTriggerPressure = static_cast<short>(255.0f * (static_cast<float>(lt) / static_cast<float>(short_max)));
	}

	if (!connected_ || allow_keyboard && kb.RTriggerPressure)
	{
		pad.RTriggerPressure = kb.RTriggerPressure;
	}
	else
	{
		auto rt = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
		pad.RTriggerPressure = static_cast<short>(255.0f * (static_cast<float>(rt) / static_cast<float>(short_max)));
	}


	Uint32 buttons = 0;

	buttons |= digital_trigger(pad.LTriggerPressure, settings.trigger_threshold, Buttons_L);
	buttons |= digital_trigger(pad.RTriggerPressure, settings.trigger_threshold, Buttons_R);

	if (connected_)
	{
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
	}

	if (allow_keyboard)
	{
		buttons |= kb.HeldButtons;
	}

	update_buttons(pad, buttons);
}

void DreamPad::set_active_motor(Motor motor, bool enable)
{
	if (effect_id == -1 || haptic == nullptr)
	{
		return;
	}

	if (settings.mega_rumble)
	{
		motor = Motor::both;
	}

	const float f = settings.rumble_factor;

	if (motor & Motor::large)
	{
		effect.leftright.large_magnitude = enable ? static_cast<ushort>(std::numeric_limits<ushort>::max() * f) : 0;
		rumble_state = static_cast<Motor>(enable ? rumble_state | motor : rumble_state & ~Motor::large);
	}

	if (motor & Motor::small)
	{
		effect.leftright.small_magnitude = enable ? static_cast<ushort>(std::numeric_limits<ushort>::max() * f) : 0;
		rumble_state = static_cast<Motor>(enable ? rumble_state | motor : rumble_state & ~Motor::small);
	}

	SDL_HapticUpdateEffect(haptic, effect_id, &effect);
	SDL_HapticRunEffect(haptic, effect_id, 1);
}

void DreamPad::copy(ControllerData& dest) const
{
	dest = pad;
}

inline int DreamPad::digital_trigger(ushort trigger, ushort threshold, int button)
{
	return trigger > threshold ? button : 0;
}

float DreamPad::convert_axes(NJS_POINT2I* dest, const NJS_POINT2I& source, short deadzone, bool radial)
{
	if (abs(source.x) < deadzone && abs(source.y) < deadzone)
	{
		*dest = {};
		return 0.0f;
	}

	constexpr short short_max = std::numeric_limits<short>::max();

	const float x = static_cast<float>(clamp<short>(source.x, -short_max, short_max));
	const float y = static_cast<float>(clamp<short>(source.y, -short_max, short_max));

	const float m = sqrt(x * x + y * y);

	const float nx = (m < deadzone) ? 0 : (x / m); // Normalized (X)
	const float ny = (m < deadzone) ? 0 : (y / m); // Normalized (Y)
	const float n  = (min(static_cast<float>(short_max), m) - deadzone) / static_cast<float>(short_max - deadzone);

	if (!radial && abs(source.x) < deadzone)
	{
		dest->x = 0;
	}
	else
	{
		dest->x = clamp<short>(static_cast<short>(128 * (nx * n)), -127, 127);
	}

	if (!radial && abs(source.y) < deadzone)
	{
		dest->y = 0;
	}
	else
	{
		dest->y = clamp<short>(static_cast<short>(128 * (ny * n)), -127, 127);
	}

	return n;
}

void DreamPad::update_buttons(ControllerData& pad, Uint32 buttons)
{
	pad.HeldButtons     = buttons;
	pad.NotHeldButtons  = ~buttons;
	pad.ReleasedButtons = pad.Old & (buttons ^ pad.Old);
	pad.PressedButtons  = buttons & (buttons ^ pad.Old);
	pad.Old             = pad.HeldButtons;
}

DreamPad::Settings::Settings()
{
	allow_keyboard    = false;
	deadzone_l        = GAMEPAD_LEFT_THUMB_DEADZONE;
	deadzone_r        = GAMEPAD_RIGHT_THUMB_DEADZONE;
	trigger_threshold = GAMEPAD_TRIGGER_THRESHOLD;
	radial_l          = true;
	radial_r          = false;
	rumble_factor     = 1.0f;
	mega_rumble       = false;
	rumble_min_time   = 0;
}
void DreamPad::Settings::apply(short deadzone_l, short deadzone_r, bool radial_l, bool radial_r, uint8 trigger_threshold,
							   float rumble_factor, bool mega_rumble, ushort rumble_min_time)
{
	this->deadzone_l        = clamp(deadzone_l, static_cast<short>(0), static_cast<short>(std::numeric_limits<short>::max()));
	this->deadzone_r        = clamp(deadzone_r, static_cast<short>(0), static_cast<short>(std::numeric_limits<short>::max()));
	this->radial_l          = radial_l;
	this->radial_r          = radial_r;
	this->trigger_threshold = trigger_threshold;
	this->rumble_factor     = rumble_factor;
	this->mega_rumble       = mega_rumble;
	this->rumble_min_time   = rumble_min_time;
}
