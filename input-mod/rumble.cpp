#include "stdafx.h"

#include <SADXModLoader.h>

#include "minmax.h"
#include "input.h"
#include "rumble.h"

namespace rumble
{
	bool cutsceneRumble = true;

	static ObjectMaster* Instances[2] = {};

	Sint32 __cdecl pdVibMxStop_hook(Uint32 port)
	{
		for (Uint32 i = 0; i < GAMEPAD_COUNT; i++)
		{
			DreamPad& pad = DreamPad::Controllers[i];
			if (pad.GetActiveMotor() != Motor::None)
				pad.SetActiveMotor(Motor::Both, 0);
		}
		
		return 0;
	}

	static void __cdecl Rumble_Main_hook(ObjectMaster* _this)
	{
		ObjUnknownB* v1 = (ObjUnknownB*)_this->UnknownB_ptr;
		PDS_VIBPARAM* param = (PDS_VIBPARAM*)_this->UnknownA_ptr;
		Motor motor = (Motor)param->reserved[0];
		DreamPad& pad = DreamPad::Controllers[param->unit];

		if (!v1->Mode)
		{
			Uint32 time = (Uint32)(v1->Time * (1000.0f / (60.0f / (float)FrameIncrement)));

			if (input::debug)
				PrintDebug("[Input] [%u] Rumble %u: %s, %u frames (%ums)\n", FrameCounter, param->unit, (motor == Motor::Small ? "R" : "L"), v1->Time, time);

			pad.SetActiveMotor(motor, time);
			v1->Mode = 1;
			Instances[(int)motor - 1] = _this;
		}

		if (v1->Time-- <= 0)
		{
			DeleteObject_(_this);
			pad.SetActiveMotor(motor, 0);
		}
	}

	static void __cdecl Rumble_Delete(ObjectMaster* _this)
	{
		PDS_VIBPARAM* param = (PDS_VIBPARAM*)_this->UnknownA_ptr;
		Motor motor = (Motor)param->reserved[0];
		Instances[(int)motor - 1] = nullptr;
	}

	static void __cdecl Rumble_Load_hook(Uint32 port, Uint32 time, Motor motor)
	{
		if (port >= GAMEPAD_COUNT)
		{
			for (ushort i = 0; i < GAMEPAD_COUNT; i++)
				Rumble_Load_hook(i, time, motor);
			return;
		}

		ObjectMaster* _this = LoadObject(LoadObjFlags_UnknownB, 0, Rumble_Main_hook);

		if (_this == nullptr)
			return;

		Uint32 time_scaled = max(4 * time, (uint)(DreamPad::Controllers[port].settings.rumbleMinTime / (1000.0f / 60.0f)));
		((ObjUnknownB*)_this->UnknownB_ptr)->Time = time_scaled;
		PDS_VIBPARAM* param = (PDS_VIBPARAM*)AllocateMemory(sizeof(PDS_VIBPARAM));

		if (param)
		{
			param->unit = port;
			param->flag = PDD_VIB_FLAG_CONTINUOUS;
			param->power = 10;
			param->freq = 50;
			param->inc = 0;

			// hax
			param->reserved[0] = motor;

			_this->DeleteSub = Rumble_Delete;
			_this->UnknownA_ptr = param;

			int i = (int)motor - 1;
			if (Instances[i] != nullptr)
				DeleteObject_(Instances[i]);
		}
		else
		{
			DeleteObject_(_this);
		}
	}

	void __cdecl RumbleA(Uint32 port, Uint32 time)
	{
		if (!cutsceneRumble && CutscenePlaying)
			return;

		if (RumbleEnabled && input::_ControllerEnabled[port])
			Rumble_Load_hook(port, clamp(time, 1u, 255u), Motor::Large);
	}

	void __cdecl RumbleB(Uint32 port, Uint32 time, int a3, int a4)
	{
		Uint32 idk; // ecx@4
		int _a3; // eax@12
		int _time; // eax@16

		if (!cutsceneRumble && CutscenePlaying)
			return;

		if (RumbleEnabled && input::_ControllerEnabled[port])
		{
			idk = time;
			if ((signed int)time <= 4)
			{
				if ((signed int)time >= -4)
				{
					if (time == 1)
					{
						idk = 2;
					}
					else if (time == -1)
					{
						idk = -2;
					}
				}
				else
				{
					idk = -4;
				}
			}
			else
			{
				idk = 4;
			}
			_a3 = a3;
			if (a3 <= 59)
			{
				if (a3 < 7)
				{
					_a3 = 7;
				}
			}
			else
			{
				_a3 = 59;
			}
			_time = a4 * _a3 / (signed int)(4 * idk);
			if (_time <= 0)
			{
				_time = 1;
			}
			Rumble_Load_hook(port, _time, Motor::Small);
		}
	}

	static const void* loc_0042D534 = (const void*)0x0042D534;
	void __declspec(naked) DefaultRumble()
	{
		__asm
		{
			mov [esp + 26Ah], 1
			jmp loc_0042D534
		}
	}
}