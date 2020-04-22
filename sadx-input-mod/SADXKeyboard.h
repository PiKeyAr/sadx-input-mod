#pragma once

#include <string>
#include <SADXStructs.h>

struct KeyboardKey
{
	char held;
	char old;
	char pressed;
};

DataArray(KeyboardKey, KeyboardKeys, 0x03B0E3E0, 256);

struct KeyData
{
	Uint32 WindowsCode;
	Uint32 SADX2004Code;
	Uint32 SADXSteamCode;
};

KeyData SADX2004Keys[] = {
	{ VK_ESCAPE, 41, 1 },
	{ VK_F1, 58, 59 },
	{ VK_F2, 59, 60 },
	{ VK_F3, 60, 61 },
	{ VK_F4, 61, 62 },
	{ VK_F5, 62, 63 },
	{ VK_F6, 63, 64 },
	{ VK_F7, 64, 65 },
	{ VK_F8, 65, 66 },
	{ VK_F9, 66, 67 },
	{ VK_F10, 67, 68 },
	{ VK_F11, 68, 87 },
	{ VK_F12, 69, 88 },
	{ VK_SNAPSHOT, 70, 183 },
	{ VK_SCROLL, 71, 70 },
	{ VK_PAUSE, 72, 197 },
	{ 0xC0, 140, 41 }, //~ (doesn't exist in vanilla SADX)
	{ 0x31, 30, 2 }, //1
	{ 0x32, 31, 3 }, //2
	{ 0x33, 32, 4 }, //3
	{ 0x34, 33, 5 }, //4
	{ 0x35, 34, 6 }, //5
	{ 0x36, 35, 7 }, //6
	{ 0x37, 36, 8 }, //7
	{ 0x38, 37, 9 }, //8
	{ 0x39, 38, 10 }, //9
	{ 0x30, 39, 11 }, //0
	{ VK_OEM_MINUS, 45, 12 }, //-
	{ 0xBB, 46, 12 }, //=
	{ VK_BACK, 42, 14 }, //Backspace
	{ VK_INSERT, 73, 210 },
	{ VK_HOME, 74, 199 },
	{ VK_PRIOR, 75, 201 }, //Page Up
	{ VK_NUMLOCK, 83, 69 }, //Num Lock
	{ 0x6F, 84, 181 }, //Num slash
	{ 0x6A, 85, 55 }, //Num asterisk
	{ 0x6D, 86, 74 }, //Num minus
	{ VK_TAB, 43, 15 },
	{ 0x51, 20, 16 }, //Q
	{ 0x57, 26, 17  }, //W
	{ 0x45, 8, 18 }, //E
	{ 0x52, 21, 19  }, //R
	{ 0x54, 23, 20 }, //T
	{ 0x59, 28, 21 }, //Y
	{ 0x55, 24, 22 }, //U
	{ 0x49, 12, 23 }, //I
	{ 0x4F, 18, 24 }, //O
	{ 0x50, 19, 25  }, //P
	{ 219, 141, 26 }, //[ (doesn't exist in vanilla SADX)
	{ 221, 142, 27 }, //] (doesn't exist in vanilla SADX)
	{ 220, 135, 43 }, //Backslash
	{ VK_DELETE, 76, 211 },
	{ VK_END, 77, 207 },
	{ VK_NEXT, 78, 209  }, //Page Down
	{ VK_NUMPAD7, 95, 71 }, //Num 7 (Home)
	{ VK_NUMPAD8, 96, 72 }, //Num 8 (Up)
	{ VK_NUMPAD9, 97, 73 }, //Num 9 (Page Up)
	{ VK_ADD, 87, 78 }, //Num Plus
	{ VK_CAPITAL, 143, 58 }, //Caps Lock (doesn't exist in vanilla SADX)
	{ 0x41, 4, 30 }, //A
	{ 0x53, 22, 31 }, //S
	{ 0x44, 7, 32 }, //D
	{ 0x46, 9, 33 }, //F
	{ 0x47, 10, 34 }, //G
	{ 0x48, 11, 35 }, //H
	{ 0x4A, 13, 36 }, //J
	{ 0x4B, 14, 37 }, //K
	{ 0x4C, 15, 38 }, //L
	{ VK_OEM_1, 51, 39 }, //Semicolon
	{ VK_OEM_7, 52, 40 }, //'
	{ VK_RETURN, 40, 28 }, //Enter
	{ VK_NUMPAD4, 92, 75 }, //Num 4 (Left)
	{ VK_NUMPAD5, 93, 76 }, //Num 5
	{ VK_NUMPAD6, 94, 77 }, //Num 6 (Right)
	{ VK_LSHIFT, 143, 42 }, //Left Shift (doesn't exist in vanilla SADX)
	{ 0x5A, 29, 44 }, //Z
	{ 0x58, 27, 45 }, //X
	{ 0x43, 6, 46 }, //C
	{ 0x56, 25, 47 }, //V
	{ 0x42, 5, 48 }, //B
	{ 0x4E, 17, 49 }, //N
	{ 0x4D, 16, 50 }, //M
	{ VK_OEM_COMMA, 54, 51 },
	{ VK_OEM_PERIOD, 55, 52 },
	{ VK_OEM_2, 56, 53 }, //Slash
	{ VK_RSHIFT, 144, 54 }, //Right Shift (doesn't exist in vanilla SADX)
	{ VK_UP, 82, 200 }, //Up
	{ VK_NUMPAD1, 89, 79 }, //Num 1 (End)
	{ VK_NUMPAD2, 90, 80 }, //Num 2 (Down)
	{ VK_NUMPAD3, 91, 81 }, //Num 3 (Page Down)
	{ VK_LCONTROL, 145, 29 }, //Left Control (doesn't exist in vanilla SADX)
	{ VK_LMENU, 146, 56 }, //Left Alt (doesn't exist in vanilla SADX)
	{ VK_SPACE, 44, 57 },
	{ VK_RMENU, 147, 184 }, //Right Alt (doesn't exist in vanilla SADX)
	{ VK_APPS, 148, 221 }, //Menu (doesn't exist in vanilla SADX)
	{ VK_RCONTROL, 149, 157 }, //Right Control (doesn't exist in vanilla SADX)
	{ VK_LEFT, 80, 203 },
	{ VK_DOWN, 81, 208 },
	{ VK_RIGHT, 79, 205 },
	{ VK_NUMPAD0, 98, 82 },
	{ 110, 150, 83 }, //Numpad Delete (doesn't exist in vanilla SADX)
	{ 256, 151, 156 }, //Numpad Enter (doesn't exist in vanilla SADX)
};