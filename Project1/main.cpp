#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Window.h"

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

int WINAPI WinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    Window window(hInstance, nCmdShow);

    if (!window.Initialize(L"DirectX Lab - Part One", 800, 600)) {
        MessageBox(NULL, L"Не удалось создать окно!", L"Ошибка", MB_OK);
        return 1;
    }

    MSG msg = {};
    bool running = true;

    // Для отслеживания состояния
    bool aPressed = false;
    bool spacePressed = false;
    bool lmbPressed = false;

    while (running) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        window.GetInputDevice().Update();

        // 1. Проверка клавиши A (нажата/отпущена)
        bool aNowPressed = window.GetInputDevice().IsKeyDown('A');
        if (aNowPressed && !aPressed) {
            SetWindowText(window.GetHandle(), L"A нажата!");
            aPressed = true;
        }
        if (!aNowPressed && aPressed) {
            SetWindowText(window.GetHandle(), L"A отпущена!");
            aPressed = false;
        }

        // 2. Проверка ПРОБЕЛА (зажат)
        bool spaceNowPressed = window.GetInputDevice().IsKeyDown(VK_SPACE);
        if (spaceNowPressed && !spacePressed) {
            SetWindowText(window.GetHandle(), L"ПРОБЕЛ нажат!");
            spacePressed = true;
        }
        if (!spaceNowPressed && spacePressed) {
            SetWindowText(window.GetHandle(), L"DirectX Lab - Part One");
            spacePressed = false;
        }

        // 3. Проверка мыши
        bool lmbNowPressed = window.GetInputDevice().IsMouseButtonDown(0);
        if (lmbNowPressed && !lmbPressed) {
            SetWindowText(window.GetHandle(), L"Левая кнопка мыши нажата!");
            lmbPressed = true;
        }
        if (!lmbNowPressed && lmbPressed) {
            SetWindowText(window.GetHandle(), L"DirectX Lab - Part One");
            lmbPressed = false;
        }

        // 4. Показ координат мыши в заголовке (всегда)
        int mouseX, mouseY;
        window.GetInputDevice().GetMousePosition(mouseX, mouseY);

        wchar_t title[256];
        swprintf_s(title, L"Mouse: %d, %d", mouseX, mouseY);

        // Если нет активных сообщений, показываем координаты
        if (!aNowPressed && !spaceNowPressed && !lmbNowPressed) {
            SetWindowText(window.GetHandle(), title);
        }

        Sleep(10);
    }

    return (int)msg.wParam;
}