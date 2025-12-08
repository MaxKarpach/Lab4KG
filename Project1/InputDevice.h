#pragma once
#include <windows.h>
#include <unordered_map>

class InputDevice {
public:
    InputDevice();

    void Update();  // Вызывать каждый кадр в начале игрового цикла
    void HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Клавиатура
    bool IsKeyDown(int keyCode) const;
    bool IsKeyPressed(int keyCode) const;
    bool IsKeyReleased(int keyCode) const;

    // Мышь
    void GetMousePosition(int& x, int& y) const;
    bool IsMouseButtonDown(int button) const;

private:
    // Состояния клавиш
    std::unordered_map<int, bool> currentKeys;
    std::unordered_map<int, bool> previousKeys;

    // Мышь
    int mouseX, mouseY;
    bool mouseButtons[3];  // 0 = левая кнопка, 1 = правая, 2 = средняя
};