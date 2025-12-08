#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "InputDevice.h"

class Window {
public:
    Window(HINSTANCE hInstance, int nCmdShow);
    ~Window();

    bool Initialize(const wchar_t* title, int width, int height);
    HWND GetHandle() const { return hWnd; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

    InputDevice& GetInputDevice() { return inputDevice; }

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message,
        WPARAM wParam, LPARAM lParam);

private:
    HINSTANCE hInstance;
    HWND hWnd;
    int width;
    int height;
    InputDevice inputDevice;
};