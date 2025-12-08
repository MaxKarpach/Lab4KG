#include "DirectXApp.h"
#include <d3d12.h>
#include <dxgi1_6.h>

DirectXApp::DirectXApp(Window& window)
    : window(window), device(nullptr), dxgiFactory(nullptr) {
}

DirectXApp::~DirectXApp() {
    if (device) device->Release();
    if (dxgiFactory) dxgiFactory->Release();
}

bool DirectXApp::Initialize() {
    UINT dxgiFactoryFlags = 0;

    // 1. Создаем DXGI фабрику
    if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)))) {
        MessageBox(NULL, L"Не удалось создать DXGI фабрику", L"Ошибка", MB_OK);
        return false;
    }

    // 2. Ищем совместимый адаптер (видеокарту)
    IDXGIAdapter1* adapter = nullptr;
    ID3D12Device* tempDevice = nullptr;

    for (UINT adapterIndex = 0;
        dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND;
        ++adapterIndex) {

        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // Пропускаем программный адаптер (WARP)
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            adapter->Release();
            continue;
        }

        // Пытаемся создать устройство
        HRESULT hr = D3D12CreateDevice(
            adapter,
            D3D_FEATURE_LEVEL_12_0,
            IID_PPV_ARGS(&tempDevice)
        );

        if (SUCCEEDED(hr)) {
            // Нашли рабочее устройство
            device = tempDevice;

            adapter->Release();
            return true;
        }

        adapter->Release();
    }

    // 3. Если не нашли аппаратный адаптер, пробуем WARP (программный)
    if (FAILED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)))) {
        MessageBox(NULL, L"Не удалось найти WARP адаптер", L"Ошибка", MB_OK);
        return false;
    }

    if (FAILED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)))) {
        MessageBox(NULL, L"Не удалось создать устройство даже через WARP", L"Ошибка", MB_OK);
        adapter->Release();
        return false;
    }

    MessageBox(NULL, L"Используется WARP (программный) адаптер", L"Внимание", MB_OK);
    adapter->Release();
    return true;
}

void DirectXApp::Render() {
    // Пока пусто - будет на шаге 8
}