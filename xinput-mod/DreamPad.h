#pragma once

#define GAMEPAD_COUNT 4
// XInput default deadzones blatantly copied, pasted, and renamed.
#define GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define GAMEPAD_TRIGGER_THRESHOLD    30

#include "SDL.h"
#include <SADXModLoader.h>
#include "typedefs.h"

enum Motor : int8
{
	None,
	Large,
	Small,
	Both
};

class DreamPad
{
public:
	DreamPad();
	~DreamPad();

	bool Open(int id);
	void Close();

	void Update();
	void Poll();
	void UpdateMotor();

	// Poor man's properties
	Motor GetActiveMotor() const { return rumbleState; }
	void SetActiveMotor(Motor motor, short magnitude);
	bool Connected() const { return isConnected; }
	int ControllerID() const { return controller_id; }

	void Copy(ControllerData& dest) const;

	struct Settings
	{
		Settings();

		short	deadzoneL;			// Left stick deadzone
		bool	radialL;			// Indicates if the stick is fully radial or semi-radial
		short	deadzoneR;			// Right stick deadzone
		bool	radialR;			// Indicates if the stick is fully radial or semi-radial
		uint8	triggerThreshold;	// Trigger threshold
		float	rumbleFactor;		// Rumble intensity multiplier (1.0 by default)
		float	scaleFactor;		// Axis scale factor (1.5 (192) by default)

		void apply(short deadzoneL, short deadzoneR,
			bool radialL, bool radialR, uint8 triggerThreshold,
			float rumbleFactor, float scaleFactor);
	} settings;


	static int DigitalTrigger(ushort trigger, ushort threshold, int button);
	static DreamPad Controllers[GAMEPAD_COUNT];

private:
	bool isConnected = false;
	int controller_id;
	SDL_GameController* gamepad;

	SDL_Haptic* haptic;
	SDL_HapticEffect effect;
	int effect_id;
	uint rumbleTime_L;
	uint rumbleTime_S;
	Motor rumbleState;

	ControllerData pad;
};

enum PDD_DEV_SUPPORT : uint32_t
{
	//	Right stick Y
	PDD_DEV_SUPPORT_AY2 = (1 << 21),
	//	Right stick X
	PDD_DEV_SUPPORT_AX2 = (1 << 20),
	//	Left stick Y
	PDD_DEV_SUPPORT_AY1 = (1 << 19),
	//	Left stick X
	PDD_DEV_SUPPORT_AX1 = (1 << 18),
	//	Analog trigger L
	PDD_DEV_SUPPORT_AL = (1 << 17),
	//	Analog trigger R
	PDD_DEV_SUPPORT_AR = (1 << 16),
	//	D-Pad B Right
	PDD_DEV_SUPPORT_KRB = (1 << 15),
	//	D-Pad B Left
	PDD_DEV_SUPPORT_KLB = (1 << 14),
	//	D-Pad B Down
	PDD_DEV_SUPPORT_KDB = (1 << 13),
	//	D-Pad B Up
	PDD_DEV_SUPPORT_KUB = (1 << 12),
	//	D button
	PDD_DEV_SUPPORT_TD = (1 << 11),
	//	X button
	PDD_DEV_SUPPORT_TX = (1 << 10),
	//	Y button
	PDD_DEV_SUPPORT_TY = (1 << 9),
	//	Z button
	PDD_DEV_SUPPORT_TZ = (1 << 8),
	//	D-Pad A Right
	PDD_DEV_SUPPORT_KR = (1 << 7),
	//	D-Pad A Left
	PDD_DEV_SUPPORT_KL = (1 << 6),
	//	D-Pad A Down
	PDD_DEV_SUPPORT_KD = (1 << 5),
	//	D-Pad A Up
	PDD_DEV_SUPPORT_KU = (1 << 4),
	//	Start button
	PDD_DEV_SUPPORT_ST = (1 << 3),
	//	A button
	PDD_DEV_SUPPORT_TA = (1 << 2),
	//	B button
	PDD_DEV_SUPPORT_TB = (1 << 1),
	//	C button
	PDD_DEV_SUPPORT_TC = (1 << 0),
};
