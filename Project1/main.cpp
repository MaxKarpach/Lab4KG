#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Window.h"
#include "DirectXApp.h"

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

int WINAPI WinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow) {

    (void)hPrevInstance;
    (void)lpCmdLine;

    Window window(hInstance, nCmdShow);
    if (!window.Initialize(L"DirectX 12 Lab", 800, 600)) {
        MessageBox(NULL, L"Failed to create window", L"Error", MB_OK);
        return 1;
    }

    DirectXApp dxApp(window);

    MSG msg = {};
    bool running = true;
    bool dxInitialized = false;

    while (running) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!dxInitialized) {
            if (!dxApp.Initialize()) {
                MessageBox(NULL, L"DirectX initialization failed", L"Error", MB_OK);
                return 1;
            }
            dxInitialized = true;
        }

        window.GetInputDevice().Update();

        if (window.GetInputDevice().IsKeyDown(VK_ESCAPE)) {
            DestroyWindow(window.GetHandle());
        }

        Sleep(16);
    }

    return 0;
}