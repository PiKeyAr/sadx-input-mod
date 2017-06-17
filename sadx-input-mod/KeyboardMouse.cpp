#include "stdafx.h"
#include "KeyboardMouse.h"

DataPointer(int, MouseMode, 0x03B0EAE0);
DataPointer(int, CursorY, 0x03B0E990);
DataPointer(int, CursorX, 0x03B0E994);
DataPointer(int, CursorMagnitude, 0x03B0E998);
DataPointer(int, CursorCos, 0x03B0E99C);
DataPointer(int, CursorSin, 0x03B0E9A0);
DataPointer(HWND, hWnd, 0x3D0FD30);

ControllerData KeyboardMouse::pad           = {};
float          KeyboardMouse::normalized_L  = 0.0f;
float          KeyboardMouse::normalized_R  = 0.0f;
bool           KeyboardMouse::mouse_update  = false;
NJS_POINT2I    KeyboardMouse::cursor        = {};
KeyboardStick  KeyboardMouse::sticks[2]     = {};
Sint16         KeyboardMouse::mouse_x       = 0;
Sint16         KeyboardMouse::mouse_y       = 0;
WNDPROC        KeyboardMouse::lpPrevWndFunc = nullptr;

inline void set_button(Uint32& i, Uint32 value, bool down)
{
	down ? i |= value : i &= ~value;
}

LRESULT __stdcall PollKeyboardMouse(HWND handle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return KeyboardMouse::ReadWindowMessage(handle, Msg, wParam, lParam);
}

void KeyboardMouse::Poll()
{
	HookWndProc();

	sticks[0].update();
	sticks[1].update();
	NJS_POINT2I stick;

	if (sticks[0].x || sticks[0].y)
	{
		ResetCursor();
		stick = static_cast<NJS_POINT2I>(sticks[0]);
	}
	else
	{
		stick = cursor;
	}

	DreamPad::UpdateButtons(pad, pad.HeldButtons);

	pad.LeftStickX       = stick.x;
	pad.LeftStickY       = stick.y;
	pad.RightStickX      = sticks[1].x;
	pad.RightStickY      = sticks[1].y;
	pad.LTriggerPressure = !!(pad.HeldButtons & Buttons_L) ? 255 : 0;
	pad.RTriggerPressure = !!(pad.HeldButtons & Buttons_R) ? 255 : 0;
}

void KeyboardMouse::UpdateKeyboardButtons(Uint32 key, bool down)
{
	switch (key)
	{
		default:
			break;

		case 'X':
		case VK_SPACE:
			set_button(pad.HeldButtons, Buttons_A, down);
			break;
		case 'Z':
			set_button(pad.HeldButtons, Buttons_B, down);
			break;
		case 'A':
			set_button(pad.HeldButtons, Buttons_X, down);
			break;
		case 'S':
			set_button(pad.HeldButtons, Buttons_Y, down);
			break;
		case 'Q':
			set_button(pad.HeldButtons, Buttons_L, down);
			break;
		case 'W':
			set_button(pad.HeldButtons, Buttons_R, down);
			break;
		case VK_RETURN:
			set_button(pad.HeldButtons, Buttons_Start, down);
			break;
		case 'D':
			set_button(pad.HeldButtons, Buttons_Z, down);
			break;
		case 'C':
			set_button(pad.HeldButtons, Buttons_C, down);
			break;
		case 'E':
			set_button(pad.HeldButtons, Buttons_D, down);
			break;

			// D-Pad
		case VK_NUMPAD8:
			set_button(pad.HeldButtons, Buttons_Up, down);
			break;
		case VK_NUMPAD5:
			set_button(pad.HeldButtons, Buttons_Down, down);
			break;
		case VK_NUMPAD4:
			set_button(pad.HeldButtons, Buttons_Left, down);
			break;
		case VK_NUMPAD6:
			set_button(pad.HeldButtons, Buttons_Right, down);
			break;

			// Left stick
		case VK_UP:
			set_button(sticks[0].directions, Buttons_Up, down);
			break;
		case VK_DOWN:
			set_button(sticks[0].directions, Buttons_Down, down);
			break;
		case VK_LEFT:
			set_button(sticks[0].directions, Buttons_Left, down);
			break;
		case VK_RIGHT:
			set_button(sticks[0].directions, Buttons_Right, down);
			break;

			// Right stick
		case 'I':
			set_button(sticks[1].directions, Buttons_Up, down);
			break;
		case 'K':
			set_button(sticks[1].directions, Buttons_Down, down);
			break;
		case 'J':
			set_button(sticks[1].directions, Buttons_Left, down);
			break;
		case 'L':
			set_button(sticks[1].directions, Buttons_Right, down);
			break;
	}
}

void KeyboardMouse::UpdateCursor(Sint32 xrel, Sint32 yrel)
{
	if (!mouse_update)
	{
		return;
	}

	CursorX = clamp(CursorX + xrel, -200, 200);
	CursorY = clamp(CursorY + yrel, -200, 200);

	auto& x = CursorX;
	auto& y = CursorY;

	auto m = x * x + y * y;

	if (m <= 625)
	{
		CursorMagnitude = 0;
		return;
	}

	CursorMagnitude = m / 361;

	if (CursorMagnitude >= 1)
	{
		if (CursorMagnitude > 120)
		{
			CursorMagnitude = 127;
		}
	}
	else
	{
		CursorMagnitude = 1;
	}

	njPushMatrix((NJS_MATRIX*)0x0389D650);
	njRotateZ(nullptr, NJM_RAD_ANG(atan2(x, y)));

	NJS_VECTOR v = { 0.0f, (float)CursorMagnitude * 1.2f, 0.0f };
	njCalcVector(nullptr, &v, &v);

	CursorCos = (int)v.x;
	CursorSin = (int)v.y;

	auto& p = cursor;
	p.x = (Sint16)clamp((int)(-v.x / 128.0f * SHRT_MAX), -SHRT_MAX, SHRT_MAX);
	p.y = (Sint16)clamp((int)(v.y / 128.0f * SHRT_MAX), -SHRT_MAX, SHRT_MAX);

	njPopMatrix(1);
}

void KeyboardMouse::ResetCursor()
{
	CursorMagnitude = 0;
	CursorCos       = 0;
	CursorSin       = 0;
	CursorX         = 0;
	CursorY         = 0;
	cursor          = {};
	mouse_update    = false;
}

void KeyboardMouse::UpdateMouseButtons(Uint32 button, bool down)
{
	switch (button)
	{
		case VK_LBUTTON:
			if (!down && !MouseMode)
			{
				ResetCursor();
			}
			mouse_update = down;
			break;

		case VK_RBUTTON:
			if (mouse_update)
			{
				set_button(pad.HeldButtons, Buttons_A, down);
			}
			else
			{
				set_button(pad.HeldButtons, Buttons_B, down);
			}
			break;

		case VK_MBUTTON:
			set_button(pad.HeldButtons, Buttons_Start, down);
			break;

		default:
			break;
	}
}

LRESULT KeyboardMouse::ReadWindowMessage(HWND handle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			UpdateMouseButtons(VK_LBUTTON, Msg == WM_LBUTTONDOWN);
			break;

		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			UpdateMouseButtons(VK_RBUTTON, Msg == WM_RBUTTONDOWN);
			break;

		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
			UpdateMouseButtons(VK_MBUTTON, Msg == WM_MBUTTONDOWN);
			break;

		case WM_MOUSEMOVE:
		{
			auto x = (short)(lParam & 0xFFFF);
			auto y = (short)(lParam >> 16);

			UpdateCursor(x - mouse_x, y - mouse_y);

			mouse_x = x;
			mouse_y = y;
			break;
		}

		case WM_MOUSEWHEEL:
			break; // TODO

		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_KEYUP:
			UpdateKeyboardButtons(wParam, Msg == WM_KEYDOWN || Msg == WM_SYSKEYDOWN);
			break;

		default:
			break;
	}

	return CallWindowProc(lpPrevWndFunc, handle, Msg, wParam, lParam);
}

void KeyboardMouse::HookWndProc()
{
	if (lpPrevWndFunc == nullptr)
	{
		lpPrevWndFunc = (WNDPROC)SetWindowLong(hWnd, GWL_WNDPROC, (LONG)PollKeyboardMouse);
	}
}
