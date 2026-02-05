#include "pch.h"
#include "mouseDelta.h"

//reduces size for compilation
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <unordered_map>

//for debugging - output to file
#include <fstream>
#include <string>

//debug log function
void DebugLog(const std::string& message)
{
    static std::ofstream logFile("TwoMice_Debug.txt", std::ios::app);
    if (logFile.is_open())
    {
        logFile << message << std::endl;
        logFile.flush();
    }
}

//
//  RAW INPUT STATE
//

//structure to hold delta values
struct MouseState {
    int dx = 0;
    int dy = 0;

    bool leftDown = false;
    bool leftPressed = false;
    bool leftReleased = false;

    bool rightDown = false;
    bool rightPressed = false;
    bool rightReleased = false;
};

//map to hold mouse handle and their corresponding delta values
static std::unordered_map<HANDLE, MouseState> mouseData;

//store the first two mice window reports
//handle kinda like a pointer
static HANDLE mouse1 = nullptr;
static HANDLE mouse2 = nullptr;

//track how many mice we've seen
static int miceDetected = 0;

//
// =====================
//  INITIALIZE RAW INPUT
// =====================
//

//initialize raw input for mice
extern "C" TWOMICE_API BOOL InitializeRawInput(HWND hwnd)
{
    DebugLog("=== InitializeRawInput called ===");

    //define a RAWINPUTDEVICE structure
    RAWINPUTDEVICE rid;
    //specify generic controls
    rid.usUsagePage = 0x01;
    //specify mouse usage id
    rid.usUsage = 0x02;
    //still used when not in foreground
    rid.dwFlags = RIDEV_INPUTSINK;
    //set target window handle
    rid.hwndTarget = hwnd;

    //if registration fails return false
    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
        DebugLog("ERROR: RegisterRawInputDevices FAILED");
        return FALSE;
    }

    DebugLog("SUCCESS: RegisterRawInputDevices succeeded");
    return TRUE;
}


//
//  HANDLE RAW INPUT
//

//handle WM_INPUT messages
extern "C" TWOMICE_API void HandleRawInput(LPARAM lParam)
{
    //size variable to hold size of raw input data
    UINT size = 0;

    //get raw input data 
    UINT rawInputDataresult =
        GetRawInputData(
            (HRAWINPUT)lParam,
            RID_INPUT,
            nullptr,
            &size,
            sizeof(RAWINPUTHEADER)
        );

    if (rawInputDataresult == (UINT)-1)
    {
        DebugLog("ERROR: GetRawInputData failed to get size");
        return;
    }

    //need buffer of type RAWINPUT to hold raw input data, size is returned from first call
    BYTE* rawBuffer = new BYTE[size];
    RAWINPUT* buffer = reinterpret_cast<RAWINPUT*>(rawBuffer);

    //call again to fill buffer
    UINT result = GetRawInputData(
        (HRAWINPUT)lParam,
        RID_INPUT,
        buffer,
        &size,
        sizeof(RAWINPUTHEADER)
    );

    if (result == (UINT)-1)
    {
        DebugLog("ERROR: GetRawInputData failed to fill buffer");
        delete[] rawBuffer;
        return;
    }

    //if not mouse data, free buffer and return
    if (buffer->header.dwType != RIM_TYPEMOUSE)
    {
        delete[] rawBuffer;
        return;
    }

    //handle device is the mouse handle
    HANDLE device = buffer->header.hDevice;

    //CRITICAL FIX: Check the flags to see if this is relative movement
    //Raw input can report MOUSE_MOVE_RELATIVE or MOUSE_MOVE_ABSOLUTE
    //We only want relative movement for delta tracking
    if (!(buffer->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
    {
        int dx = buffer->data.mouse.lLastX;
        int dy = buffer->data.mouse.lLastY;
        USHORT buttonFlags = buffer->data.mouse.usButtonFlags;

        //assign first two mice to mouse1 and mouse2
        if (mouse1 == nullptr)
        {
            mouse1 = device;
            miceDetected++;
            DebugLog("MOUSE 1 DETECTED: Handle = " + std::to_string((long long)device));
        }
        else if (mouse2 == nullptr && device != mouse1)
        {
            mouse2 = device;
            miceDetected++;
            DebugLog("MOUSE 2 DETECTED: Handle = " + std::to_string((long long)device));
        }

        //accumulate deltas only for tracked mice
        if (device == mouse1 || device == mouse2)
        {
            auto& state = mouseData[device];

            //log first few deltas to verify accumulation
            static int logCount = 0;
            if (logCount < 10 && (dx != 0 || dy != 0))
            {
                std::string mouseNum = (device == mouse1) ? "MOUSE1" : "MOUSE2";
                DebugLog(mouseNum + " RAW DELTA: dx=" + std::to_string(dx) + " dy=" + std::to_string(dy));
                logCount++;
            }

            state.dx += dx;
            state.dy += dy;

            if (buttonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
            {
                if (!state.leftDown)
                    state.leftPressed = true;

                state.leftDown = true;
            }

            if (buttonFlags & RI_MOUSE_LEFT_BUTTON_UP)
            {
                state.leftDown = false;
                state.leftReleased = true;
            }

            if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
            {
                if (!state.rightDown)
                    state.rightPressed = true;

                state.rightDown = true;
            }

            if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
            {
                state.rightDown = false;
                state.rightReleased = true;
            }
        }
    }

    //free buffer
    delete[] rawBuffer;
}


//  PUBLIC API 

//get the accumulated delta for mouse1
extern "C" TWOMICE_API void GetMouse1Delta(int& dx, int& dy)
{
    if (mouse1 && mouseData.count(mouse1))
    {
        dx = mouseData[mouse1].dx;
        dy = mouseData[mouse1].dy;

        //log retrieval for first few calls
        static int getCount1 = 0;
        if (getCount1 < 5)
        {
            DebugLog("GetMouse1Delta: returning dx=" + std::to_string(dx) + " dy=" + std::to_string(dy));
            getCount1++;
        }

        mouseData[mouse1].dx = 0;
        mouseData[mouse1].dy = 0;
    }
    else
    {
        dx = 0;
        dy = 0;
    }
}

//get the accumulated delta for mouse2
extern "C" TWOMICE_API void GetMouse2Delta(int& dx, int& dy)
{
    if (mouse2 && mouseData.count(mouse2))
    {
        dx = mouseData[mouse2].dx;
        dy = mouseData[mouse2].dy;

        //log retrieval for first few calls
        static int getCount2 = 0;
        if (getCount2 < 5)
        {
            DebugLog("GetMouse2Delta: returning dx=" + std::to_string(dx) + " dy=" + std::to_string(dy));
            getCount2++;
        }

        mouseData[mouse2].dx = 0;
        mouseData[mouse2].dy = 0;
    }
    else
    {
        dx = 0;
        dy = 0;
    }
}

//mouse 1 left button
extern "C" TWOMICE_API bool GetMouse1LeftPressed()
{
    if (mouse1 && mouseData.count(mouse1))
    {
        bool v = mouseData[mouse1].leftPressed;
        mouseData[mouse1].leftPressed = false;
        return v;
    }
    return false;
}

extern "C" TWOMICE_API bool GetMouse1LeftDown()
{
    if (mouse1 && mouseData.count(mouse1))
        return mouseData[mouse1].leftDown;

    return false;
}

extern "C" TWOMICE_API bool GetMouse1LeftReleased()
{
    if (mouse1 && mouseData.count(mouse1))
    {
        bool v = mouseData[mouse1].leftReleased;
        mouseData[mouse1].leftReleased = false;
        return v;
    }
    return false;
}

// Mouse 1 right button
extern "C" TWOMICE_API bool GetMouse1RightPressed()
{
    if (mouse1 && mouseData.count(mouse1))
    {
        bool v = mouseData[mouse1].rightPressed;
        mouseData[mouse1].rightPressed = false;
        return v;
    }
    return false;
}

extern "C" TWOMICE_API bool GetMouse1RightDown()
{
    if (mouse1 && mouseData.count(mouse1))
        return mouseData[mouse1].rightDown;

    return false;
}

extern "C" TWOMICE_API bool GetMouse1RightReleased()
{
    if (mouse1 && mouseData.count(mouse1))
    {
        bool v = mouseData[mouse1].rightReleased;
        mouseData[mouse1].rightReleased = false;
        return v;
    }
    return false;
}

// Mouse 2 left button
extern "C" TWOMICE_API bool GetMouse2LeftPressed()
{
    if (mouse2 && mouseData.count(mouse2))
    {
        bool v = mouseData[mouse2].leftPressed;
        mouseData[mouse2].leftPressed = false;
        return v;
    }
    return false;
}

extern "C" TWOMICE_API bool GetMouse2LeftDown()
{
    if (mouse2 && mouseData.count(mouse2))
        return mouseData[mouse2].leftDown;

    return false;
}

extern "C" TWOMICE_API bool GetMouse2LeftReleased()
{
    if (mouse2 && mouseData.count(mouse2))
    {
        bool v = mouseData[mouse2].leftReleased;
        mouseData[mouse2].leftReleased = false;
        return v;
    }
    return false;
}

//mouse 2 right button
extern "C" TWOMICE_API bool GetMouse2RightPressed()
{
    if (mouse2 && mouseData.count(mouse2))
    {
        bool v = mouseData[mouse2].rightPressed;
        mouseData[mouse2].rightPressed = false;
        return v;
    }
    return false;
}

extern "C" TWOMICE_API bool GetMouse2RightDown()
{
    if (mouse2 && mouseData.count(mouse2))
        return mouseData[mouse2].rightDown;

    return false;
}

extern "C" TWOMICE_API bool GetMouse2RightReleased()
{
    if (mouse2 && mouseData.count(mouse2))
    {
        bool v = mouseData[mouse2].rightReleased;
        mouseData[mouse2].rightReleased = false;
        return v;
    }
    return false;
}