#include "stdafx.h"
#include "SDL.h"
#include <limits>
#include "minmax.h"

#include "DreamPad.h"

bool e_key[9];

static const Uint32 PAD_SUPPORT =
	PDD_DEV_SUPPORT_TA | PDD_DEV_SUPPORT_TB | PDD_DEV_SUPPORT_TX | PDD_DEV_SUPPORT_TY | PDD_DEV_SUPPORT_ST
	#ifdef EXTENDED_BUTTONS
	| PDD_DEV_SUPPORT_TC | PDD_DEV_SUPPORT_TD | PDD_DEV_SUPPORT_TZ
	#endif
	| PDD_DEV_SUPPORT_AR | PDD_DEV_SUPPORT_AL
	| PDD_DEV_SUPPORT_KU | PDD_DEV_SUPPORT_KD | PDD_DEV_SUPPORT_KL | PDD_DEV_SUPPORT_KR
	| PDD_DEV_SUPPORT_AX1 | PDD_DEV_SUPPORT_AY1 | PDD_DEV_SUPPORT_AX2 | PDD_DEV_SUPPORT_AY2;

DreamPad DreamPad::controllers[GAMEPAD_COUNT];

inline int digital_trigger(const ushort trigger, const ushort threshold, const int button)
{
	return trigger > threshold ? button : 0;
}

DreamPad::DreamPad(DreamPad&& other) noexcept
{
	move_from(std::move(other));
}

DreamPad::~DreamPad()
{
	close();
}

DreamPad& DreamPad::operator=(DreamPad&& other) noexcept
{
	move_from(std::move(other));
	return *this;
}

bool DreamPad::open(int id)
{
	if (connected_)
	{
		close();
	}

	joystick = SDL_JoystickOpen(id);

	if (joystick == nullptr)
	{
		connected_ = false;
		return false;
	}

	dc_pad.Support = PAD_SUPPORT;

	if (joystick == nullptr)
	{
		connected_ = false;
		return false;
	}

	controller_id_ = id;
	haptic = SDL_HapticOpenFromJoystick(joystick);
	numaxes = SDL_JoystickNumAxes(joystick);
	PrintDebug("Controller %d axes: %d\n", controller_id_, numaxes);

	if (haptic == nullptr)
	{
		connected_ = true;
		return true;
	}

	if (SDL_HapticRumbleSupported(haptic))
	{
		// TODO: Properly detect supported rumble types
		effect.leftright.type = SDL_HAPTIC_LEFTRIGHT;
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

	if (joystick != nullptr)
	{
		SDL_JoystickClose(joystick);
		joystick = nullptr;
	}

	controller_id_ = -1;
	connected_ = false;
}

__int16 GetAxis(PadConfigItem* item, int direction, SDL_Joystick* joystick)
{
	//Real axes
	if (item->type == PadItemType_Axis)
	{
		if (direction == -1) return min((__int16)0, SDL_JoystickGetAxis(joystick, item->index));
		else return max((__int16)0, SDL_JoystickGetAxis(joystick, item->index));
	}
	//Button axes
	else
	{
		if (SDL_JoystickGetButton(joystick, item->index)) return -32767 * direction; else return 0;
	}
}

Uint8 GetButton(PadConfigItem* item, SDL_Joystick* joystick)
{
	//PrintDebug("Button pressedL %d\n", SDL_JoystickGetButton(joystick, 6));
	Uint8 result;
	//Axes
	if (item->type == PadItemType_Axis)
	{
		if (item->direction == 1)
		{
			if (SDL_JoystickGetAxis(joystick, item->index) > 8000) result = 1; else result = 0;
		}
		else
		{
			if (SDL_JoystickGetAxis(joystick, item->index) < -8000) result = 1; else result = 0;
		}
	}
	//Buttons
	else
	{
		//PrintDebug("item %d, %d\n", item->index, SDL_JoystickGetButton(joystick, item->index));
		result = SDL_JoystickGetButton(joystick, item->index);
	}
	return result;
}

void DreamPad::poll()
{
	NJS_POINT2I axis;
	Sint16 axis_up;
	Sint16 axis_down;
	Sint16 axis_left;
	Sint16 axis_right;
	int dir_y;
	int dir_x;
	bool half_press = false;

	if (!connected_ && !settings.allow_keyboard)
	{
		return;
	}
	// TODO: keyboard/mouse toggle
	auto& kb = KeyboardMouse::dreamcast_data();
	bool allow_keyboard = settings.allow_keyboard;

	//Half-press
	if (connected_ && GetButton(&PlayerConfigs[controller_id_ + 2].h, joystick))
	{
		half_press = true;
	}

	//Stick 0
	if (!connected_ || (allow_keyboard && (kb.LeftStickX || kb.LeftStickY)))
	{
		normalized_l_ = KeyboardMouse::normalized_l();
		dc_pad.LeftStickX = kb.LeftStickX;
		dc_pad.LeftStickY = kb.LeftStickY;
	}
	else
	{
		//Get axes for stick 0
		axis_up = GetAxis(&PlayerConfigs[controller_id_ + 2].s0_up, -1, joystick);
		axis_down = GetAxis(&PlayerConfigs[controller_id_ + 2].s0_down, 1, joystick);
		axis_left = GetAxis(&PlayerConfigs[controller_id_ + 2].s0_left, -1, joystick);
		axis_right = GetAxis(&PlayerConfigs[controller_id_ + 2].s0_right, 1, joystick);
		//Combine axes for stick 0
		axis.x = axis_left + axis_right;
		axis.y = axis_up + axis_down;
		//PrintDebug("Stick 0 Up %d, Down %d, Left %d, Right %d\n", axis_up, axis_down, axis_left, axis_right);
		//Normalize stick 0
		if (PlayerConfigs[controller_id_ + 2].s0_up.direction == -1 && PlayerConfigs[controller_id_ + 2].s0_down.direction == 1) dir_y = 1; else dir_y = -1;
		if (PlayerConfigs[controller_id_ + 2].s0_left.direction == -1 && PlayerConfigs[controller_id_ + 2].s0_right.direction == 1) dir_x = 1; else dir_x = -1;
		normalized_l_ = convert_axes(reinterpret_cast<NJS_POINT2I*>(&dc_pad.LeftStickX), axis, settings.deadzone_l, settings.radial_l, dir_x, dir_y);
	}
	//Stick 1
	if (!connected_ || (allow_keyboard && (kb.RightStickX || kb.RightStickY)))
	{
		normalized_r_ = KeyboardMouse::normalized_r();
		dc_pad.RightStickX = kb.RightStickX;
		dc_pad.RightStickY = kb.RightStickY;
	}
	else
	{
		//Get axes for stick 1
		axis_up = GetAxis(&PlayerConfigs[controller_id_ + 2].s1_up, -1, joystick);
		axis_down = GetAxis(&PlayerConfigs[controller_id_ + 2].s1_down, 1, joystick);
		axis_left = GetAxis(&PlayerConfigs[controller_id_ + 2].s1_left, -1, joystick);
		axis_right = GetAxis(&PlayerConfigs[controller_id_ + 2].s1_right, 1, joystick);
		//Combine axes for stick 1
		axis.x = axis_left + axis_right;
		axis.y = axis_up + axis_down;
		//PrintDebug("Stick 1 Up %d, Down %d, Left %d, Right %d\n", axis_up, axis_down, axis_left, axis_right);
		if (PlayerConfigs[controller_id_ + 2].s1_up.direction == -1 && PlayerConfigs[controller_id_ + 2].s1_down.direction == 1) dir_y = 1; else dir_y = -1;
		if (PlayerConfigs[controller_id_ + 2].s1_left.direction == -1 && PlayerConfigs[controller_id_ + 2].s1_right.direction == 1) dir_x = 1; else dir_x = -1;
		normalized_r_ = convert_axes(reinterpret_cast<NJS_POINT2I*>(&dc_pad.RightStickX), axis, settings.deadzone_r, settings.radial_r, dir_x, dir_y);
		DisplayDebugStringFormatted(NJM_LOCATION(0, 0), "Axis 0: %d, Axis 1: %d, Axis 2: %d, Axis 4:%d", SDL_JoystickGetAxis(joystick, 0), SDL_JoystickGetAxis(joystick, 1), SDL_JoystickGetAxis(joystick, 2), SDL_JoystickGetAxis(joystick, 3));
		DisplayDebugStringFormatted(NJM_LOCATION(0, 1), "Axis 4: %d, Axis 5: %d, Axis 6: %d, Axis 7:%d", SDL_JoystickGetAxis(joystick, 4), SDL_JoystickGetAxis(joystick, 5), SDL_JoystickGetAxis(joystick, 6), SDL_JoystickGetAxis(joystick, 7));
	}
	if (half_press)
	{
		dc_pad.LeftStickX /= 2;
		dc_pad.LeftStickY /= 2;
		dc_pad.RightStickX /= 2;
		dc_pad.RightStickY /= 2;
		normalized_l_ /= 2.0f;
		normalized_r_ /= 2.0f;
	}
	constexpr short short_max = std::numeric_limits<short>::max();
	//Left trigger
	if (!connected_ || (allow_keyboard && kb.LTriggerPressure))
	{
		dc_pad.LTriggerPressure = kb.LTriggerPressure;
	}
	else
	{
		auto lt = GetAxis(&PlayerConfigs[controller_id_ + 2].lt, 1, joystick);
		dc_pad.LTriggerPressure = static_cast<short>(255.0f * (static_cast<float>(lt) / static_cast<float>(short_max)));
	}
	//Right trigger
	if (!connected_ || (allow_keyboard && kb.RTriggerPressure))
	{
		dc_pad.RTriggerPressure = kb.RTriggerPressure;
	}
	else
	{
		auto rt = GetAxis(&PlayerConfigs[controller_id_ + 2].rt, 1, joystick);
		dc_pad.RTriggerPressure = static_cast<short>(255.0f * (static_cast<float>(rt) / static_cast<float>(short_max)));
	}

	//Buttons
	Uint32 buttons = 0;

	buttons |= digital_trigger(dc_pad.LTriggerPressure, settings.trigger_threshold, Buttons_L);
	buttons |= digital_trigger(dc_pad.RTriggerPressure, settings.trigger_threshold, Buttons_R);

	if (connected_)
	{
		if (!DemoPlaying)
		{
			if (GetButton(&PlayerConfigs[controller_id_ + 2].a, joystick))
			{
				buttons |= Buttons_A;
			}
			if (GetButton(&PlayerConfigs[controller_id_ + 2].b, joystick))
			{
				buttons |= Buttons_B;
			}
			if (GetButton(&PlayerConfigs[controller_id_ + 2].x, joystick))
			{
				buttons |= Buttons_X;
			}
			if (GetButton(&PlayerConfigs[controller_id_ + 2].y, joystick))
			{
				buttons |= Buttons_Y;
			}
		}
		if (GetButton(&PlayerConfigs[controller_id_ + 2].start, joystick))
		{
			buttons |= Buttons_Start;
		}

#ifdef EXTENDED_BUTTONS
		if (GetButton(&PlayerConfigs[controller_id_ + 2].c, joystick))
		{
			buttons |= Buttons_C;
		}
		if (GetButton(&PlayerConfigs[controller_id_ + 2].d, joystick))
		{
			buttons |= Buttons_D;
		}
		if (GetButton(&PlayerConfigs[controller_id_ + 2].z, joystick))
		{
			buttons |= Buttons_Z;
		}
		if (GetButton(&PlayerConfigs[controller_id_ + 2].e, joystick))
		{
			e_key[controller_id_ + 2] = true;
		}
#endif

		if (GetButton(&PlayerConfigs[controller_id_ + 2].dpad_up, joystick))
		{
			buttons |= Buttons_Up;
			if (PlayerConfigs[controller_id_ + 2].dpad_camera) dc_pad.RightStickY = -127;
		}
		if (GetButton(&PlayerConfigs[controller_id_ + 2].dpad_down, joystick))
		{
			buttons |= Buttons_Down;
			if (PlayerConfigs[controller_id_ + 2].dpad_camera) dc_pad.RightStickY = 127;
		}
		if (GetButton(&PlayerConfigs[controller_id_ + 2].dpad_left, joystick))
		{
			buttons |= Buttons_Left;
			if (PlayerConfigs[controller_id_ + 2].dpad_camera) dc_pad.RightStickX = -127;
		}
		if (GetButton(&PlayerConfigs[controller_id_ + 2].dpad_right, joystick))
		{
			buttons |= Buttons_Right;
			if (PlayerConfigs[controller_id_ + 2].dpad_camera) dc_pad.RightStickX = 127;
		}
	}

	if (allow_keyboard)
	{
		buttons |= kb.HeldButtons;
	}
	
	update_buttons(dc_pad, buttons);
}

Motor DreamPad::active_motor() const
{
	return rumble_state;
}

bool DreamPad::connected() const
{
	return connected_;
}

int DreamPad::controller_id() const
{
	return controller_id_;
}

float DreamPad::normalized_l() const
{
	return normalized_l_;
}

float DreamPad::normalized_r() const
{
	return normalized_r_;
}

const ControllerData& DreamPad::dreamcast_data() const
{
	return dc_pad;
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

float DreamPad::convert_axes(NJS_POINT2I* dest, const NJS_POINT2I& source, short deadzone, bool radial, int direction_x, int direction_y)
{
	if (abs(source.x) < deadzone && abs(source.y) < deadzone)
	{
		*dest = {};
		return 0.0f;
	}

	constexpr short short_max = std::numeric_limits<short>::max();

	const auto x = direction_x * static_cast<float>(clamp<short>(source.x, -short_max, short_max));
	const auto y = direction_y * static_cast<float>(clamp<short>(source.y, -short_max, short_max));

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

void DreamPad::move_from(DreamPad&& other)
{
	joystick        = other.joystick;
	haptic         = other.haptic;
	effect         = other.effect;
	controller_id_ = other.controller_id_;
	effect_id      = other.effect_id;
	connected_     = other.connected_;
	rumble_state   = other.rumble_state;
	normalized_l_  = other.normalized_l_;
	normalized_r_  = other.normalized_r_;
	settings       = other.settings;

	other.joystick        = nullptr;
	other.haptic         = nullptr;
	other.controller_id_ = -1;
	other.effect_id      = -1;
	other.connected_     = false;
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

void DreamPad::Settings::set_deadzone_l(const short deadzone)
{
	this->deadzone_l = clamp<short>(deadzone, 0, std::numeric_limits<short>::max());
}

void DreamPad::Settings::set_deadzone_r(const short deadzone)
{
	this->deadzone_r = clamp<short>(deadzone, 0, std::numeric_limits<short>::max());
}
