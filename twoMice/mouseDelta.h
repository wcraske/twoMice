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

    //call from window procedure when WM_INPUT arrives
    TWOMICE_API void HandleRawInput(LPARAM lParam);

    //call to fetch deltas for mouse 1 (resets deltas after reading)
    TWOMICE_API void GetMouse1Delta(int& dx, int& dy);

    //call to fetch deltas for mouse 2 (resets deltas after reading)
    TWOMICE_API void GetMouse2Delta(int& dx, int& dy);

    // Mouse 1 left button
    TWOMICE_API bool GetMouse1LeftPressed();
    TWOMICE_API bool GetMouse1LeftDown();
    TWOMICE_API bool GetMouse1LeftReleased();

    // Mouse 1 right button
    TWOMICE_API bool GetMouse1RightPressed();
    TWOMICE_API bool GetMouse1RightDown();
    TWOMICE_API bool GetMouse1RightReleased();

    // Mouse 2 left button
    TWOMICE_API bool GetMouse2LeftPressed();
    TWOMICE_API bool GetMouse2LeftDown();
    TWOMICE_API bool GetMouse2LeftReleased();

    // Mouse 2 right button
    TWOMICE_API bool GetMouse2RightPressed();
    TWOMICE_API bool GetMouse2RightDown();
    TWOMICE_API bool GetMouse2RightReleased();
}