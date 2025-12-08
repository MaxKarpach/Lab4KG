#include "Window.h"

Window::Window(HINSTANCE hInstance, int nCmdShow)
    : hInstance(hInstance), hWnd(nullptr), width(800), height(600) {
}

Window::~Window() {
    if (hWnd) DestroyWindow(hWnd);
}

bool Window::Initialize(const wchar_t* title, int width, int height) {
    this->width = width;
    this->height = height;

    // 1. Регистрация класса окна
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"DirectXWindowClass";
    wc.hIconSm = wc.hIcon;

    if (!RegisterClassEx(&wc)) {
        return false;
    }

    // 2. Рассчитываем размер с учётом рамок окна
    RECT windowRect = { 0, 0, width, height };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // 3. Создание окна
    hWnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        L"DirectXWindowClass",
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        this  // Передаём this для доступа в WindowProc
    );

    if (!hWnd) {
        return false;
    }

    // 4. Сохраняем указатель на Window в данных окна
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // 5. Показ окна
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    return true;
}

LRESULT CALLBACK Window::WindowProc(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam) {
    // Получаем указатель на объект Window
    Window* window = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (window) {
        // Передаём сообщение в InputDevice
        window->GetInputDevice().HandleMessage(hWnd, message, wParam, lParam);
    }

    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hWnd);
            return 0;
        }
        break;

    case WM_SIZE:
        if (window) {
            window->width = LOWORD(lParam);
            window->height = HIWORD(lParam);
        }
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}