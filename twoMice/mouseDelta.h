#pragma once

#include <Windows.h>
#include <windef.h>

//export or import based on build
#ifdef TWOMICE_EXPORTS
#define TWOMICE_API __declspec(dllexport)
#else
#define TWOMICE_API __declspec(dllimport)
#endif

//dont mangle names
extern "C"
{
	//call once after creating window to receive messages
	TWOMICE_API BOOL InitializeRawInput(HWND hwnd);

	//call from window procedure to when WM_INPUT arrives
	TWOMICE_API BOOL HandleRawInput(LPARAM lParam);

	//call to fetch deltas for mouse 1
	TWOMICE_API BOOL GetMouse1Delta(int& dx, int& dy);

	//call to fetch deltas for mouse 2
	TWOMICE_API BOOL GetMouse2Delta(int& dx, int& dy);
}