#include "mouseDelta.h"
//https://learn.microsoft.com/en-us/windows/win32/inputdev/about-raw-input
// first create array of RAWINPUTDEVICE structures 
//this specifies top level collection, defined by usage page and id
//application call RegisterRawInputDevices to register for the devices	
//to get list of raw input devices call GetRawInputDeviceList
//to get device info call GetRawInputDeviceInfo
//through dwFlags, application can select spcific devices to listen or ignore
//through this we can create an application that listens to two mice and returns deltas
//https://yellowafterlife.itch.io/gamemaker-raw-input


//reduces size for compilation
#define WIN32_LEAN_AND_MEAN
//to map mouse handles to their deltas
#include <unordered_map>

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


//initialize raw input for mice
BOOL InitializeRawInput(HWND hwnd)
{
    //define a RAWINPUTDEVICE structure
    RAWINPUTDEVICE rid;
    //specifu generic controls
    rid.usUsagePage = 0x01;
    //specify mouse usage id
    rid.usUsage = 0x02;
    //still used when not in foreground
    rid.dwFlags = RIDEV_INPUTSINK;
    //set target window handle
    rid.hwndTarget = hwnd;

    //if registration fails return false
    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
        return FALSE;

    return TRUE;
}

//handle WM_INPUT messages
//LPARAM lParam contains the handle to the RAWINPUT structure, must be casted to HRAWINPUT, since same 
TWOMICE_API void HandleRawInput(LPARAM lParam)
{
    //size variable to hold size of raw input data
    UINT size = 0;
    //get raw input data 
    UINT rawInputDataresult = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
    if (rawInputDataresult == 0 || rawInputDataresult == (UINT)-1) {
        return;

    }
    //need buffer of type RAWINPUT to hold raw input data, size is returned from first call
    BYTE* rawBuffer = new BYTE[size];
    RAWINPUT* buffer = reinterpret_cast<RAWINPUT*>(rawBuffer);

    //call again to fill buffer
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER));
    //use -> since the buffer is a pointer'
    //if not mouse data, free buffer and return
    if (buffer->header.dwType != RIM_TYPEMOUSE)
    {
        delete[] rawBuffer;
        return;
    }
    //handle device is the mouse handle
    HANDLE device = buffer->header.hDevice;
    int dx = buffer->data.mouse.lLastX;
    int dy = buffer->data.mouse.lLastY;
    USHORT buttonFlags = buffer->data.mouse.usButtonFlags;


    //assign first two mice to mouse1 and mouse2
    if (mouse1 == nullptr) {
        mouse1 = device;
    }
    else if (mouse2 == nullptr && device != mouse1) {
        mouse2 = device;
    }

    //accumulate deltas only for tracked mice
    if (device == mouse1 || device == mouse2) {
        auto& state = mouseData[device];

        state.dx += dx;
        state.dy += dy;

        if (buttonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
            if (!state.leftDown)
                state.leftPressed = true;
            state.leftDown = true;
        }

        if (buttonFlags & RI_MOUSE_LEFT_BUTTON_UP) {
            state.leftDown = false;
            state.leftReleased = true;
        }

        if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
            if (!state.rightDown)
                state.rightPressed = true;
            state.rightDown = true;
        }

        if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_UP) {
            state.rightDown = false;
            state.rightReleased = true;
        }
    }

    //free buffer
    delete[] rawBuffer;
}

//get the accumulated delta for mouse1
TWOMICE_API void GetMouse1Delta(int& dx, int& dy)
{
    if (mouse1 && mouseData.count(mouse1)) {
        dx = mouseData[mouse1].dx;
        dy = mouseData[mouse1].dy;
        mouseData[mouse1].dx = 0;
        mouseData[mouse1].dy = 0;
    }
    else {
        dx = 0;
        dy = 0;
    }
}

//get the accumulated delta for mouse2
TWOMICE_API void GetMouse2Delta(int& dx, int& dy)
{
    if (mouse2 && mouseData.count(mouse2)) {
        dx = mouseData[mouse2].dx;
        dy = mouseData[mouse2].dy;
        mouseData[mouse2].dx = 0;
        mouseData[mouse2].dy = 0;
    }
    else {
        dx = 0;
        dy = 0;
    }
}

//mouse 1 left button
TWOMICE_API bool GetMouse1LeftPressed()
{
    if (mouse1 && mouseData.count(mouse1)) {
        bool v = mouseData[mouse1].leftPressed;
        mouseData[mouse1].leftPressed = false;
        return v;
    }
    return false;
}

TWOMICE_API bool GetMouse1LeftDown()
{
    if (mouse1 && mouseData.count(mouse1)) {
        return mouseData[mouse1].leftDown;
    }
    return false;
}

TWOMICE_API bool GetMouse1LeftReleased()
{
    if (mouse1 && mouseData.count(mouse1)) {
        bool v = mouseData[mouse1].leftReleased;
        mouseData[mouse1].leftReleased = false;
        return v;
    }
    return false;
}

// Mouse 1 right button
TWOMICE_API bool GetMouse1RightPressed()
{
    if (mouse1 && mouseData.count(mouse1)) {
        bool v = mouseData[mouse1].rightPressed;
        mouseData[mouse1].rightPressed = false;
        return v;
    }
    return false;
}

TWOMICE_API bool GetMouse1RightDown()
{
    if (mouse1 && mouseData.count(mouse1)) {
        return mouseData[mouse1].rightDown;
    }
    return false;
}

TWOMICE_API bool GetMouse1RightReleased()
{
    if (mouse1 && mouseData.count(mouse1)) {
        bool v = mouseData[mouse1].rightReleased;
        mouseData[mouse1].rightReleased = false;
        return v;
    }
    return false;
}

// Mouse 2 left button
TWOMICE_API bool GetMouse2LeftPressed()
{
    if (mouse2 && mouseData.count(mouse2)) {
        bool v = mouseData[mouse2].leftPressed;
        mouseData[mouse2].leftPressed = false;
        return v;
    }
    return false;
}

TWOMICE_API bool GetMouse2LeftDown()
{
    if (mouse2 && mouseData.count(mouse2)) {
        return mouseData[mouse2].leftDown;
    }
    return false;
}

TWOMICE_API bool GetMouse2LeftReleased()
{
    if (mouse2 && mouseData.count(mouse2)) {
        bool v = mouseData[mouse2].leftReleased;
        mouseData[mouse2].leftReleased = false;
        return v;
    }
    return false;
}

//mouse 2 right button
TWOMICE_API bool GetMouse2RightPressed()
{
    if (mouse2 && mouseData.count(mouse2)) {
        bool v = mouseData[mouse2].rightPressed;
        mouseData[mouse2].rightPressed = false;
        return v;
    }
    return false;
}


TWOMICE_API bool GetMouse2RightDown()
{
    if (mouse2 && mouseData.count(mouse2)) {
        return mouseData[mouse2].rightDown;
    }
    return false;
}

TWOMICE_API bool GetMouse2RightReleased()
{
    if (mouse2 && mouseData.count(mouse2)) {
        bool v = mouseData[mouse2].rightReleased;
        mouseData[mouse2].rightReleased = false;
        return v;
    }
    return false;
}
