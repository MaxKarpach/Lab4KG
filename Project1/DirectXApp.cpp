#include "DirectXApp.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

DirectXApp::DirectXApp(Window& window) : window(window) {}

DirectXApp::~DirectXApp() {
    Shutdown();
}

void DirectXApp::Shutdown() {
    // Flush командной очереди перед закрытием
    FlushCommandQueue();

    // Освобождаем буферы
    for (int i = 0; i < SwapChainBufferCount; i++) {
        mSwapChainBuffer[i].Reset();
    }

    // Освобождаем буфер глубины
    mDepthStencilBuffer.Reset();

    // Сбрасываем дескрипторные кучи
    mRtvHeap.Reset();
    mDsvHeap.Reset();

    // Сбрасываем SwapChain
    mSwapChain.Reset();

    // Закрываем список команд
    if (mCommandList) {
        mCommandList.Reset();
    }

    // Освобождение ресурсов в обратном порядке
    mFence.Reset();
    mDirectCmdListAlloc.Reset();
    mCommandQueue.Reset();
    device.Reset();
    adapter.Reset();
    dxgiFactory.Reset();
}

bool DirectXApp::CreateDXGIFactory() {
    UINT factoryFlags = 0;
    HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) {
        MessageBox(NULL, L"CreateDXGIFactory2 failed", L"Error", MB_OK);
        return false;
    }
    return true;
}

bool DirectXApp::GetHardwareAdapter() {
    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(dxgiFactory.As(&factory6))) {
        for (UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                IID_PPV_ARGS(&adapter)));
                ++adapterIndex) {

            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(),
                D3D_FEATURE_LEVEL_12_0,
                _uuidof(ID3D12Device), nullptr))) {
                return true;
            }
        }
    }
    return false;
}

bool DirectXApp::CreateD3DDevice() {
    if (!GetHardwareAdapter()) {
        if (FAILED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)))) {
            MessageBox(NULL, L"No hardware adapter found and WARP failed", L"Error", MB_OK);
            return false;
        }
        MessageBox(NULL, L"Using WARP software adapter", L"Info", MB_OK);
    }

    HRESULT hr = D3D12CreateDevice(
        adapter.Get(),
        D3D_FEATURE_LEVEL_12_0,
        IID_PPV_ARGS(&device)
    );

    if (FAILED(hr)) {
        MessageBox(NULL, L"D3D12CreateDevice failed", L"Error", MB_OK);
        return false;
    }

    return true;
}

bool DirectXApp::CreateCommandObjects() {
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

    HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));
    if (FAILED(hr)) {
        MessageBox(NULL, L"Failed to create command queue", L"Error", MB_OK);
        return false;
    }

    hr = device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&mDirectCmdListAlloc)
    );
    if (FAILED(hr)) {
        MessageBox(NULL, L"Failed to create command allocator", L"Error", MB_OK);
        return false;
    }

    hr = device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        mDirectCmdListAlloc.Get(),
        nullptr,
        IID_PPV_ARGS(&mCommandList)
    );
    if (FAILED(hr)) {
        MessageBox(NULL, L"Failed to create command list", L"Error", MB_OK);
        return false;
    }

    mCommandList->Close();
    return true;
}

bool DirectXApp::CreateFence() {
    HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
    if (FAILED(hr)) {
        MessageBox(NULL, L"Failed to create fence", L"Error", MB_OK);
        return false;
    }
    mFenceValue = 0;
    return true;
}

void DirectXApp::FlushCommandQueue() {
    mFenceValue++;
    mCommandQueue->Signal(mFence.Get(), mFenceValue);

    if (mFence->GetCompletedValue() < mFenceValue) {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        mFence->SetEventOnCompletion(mFenceValue, eventHandle);
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

bool DirectXApp::CreateSwapChain() {
    RECT clientRect;
    GetClientRect(window.GetHandle(), &clientRect);
    mClientWidth = clientRect.right - clientRect.left;
    mClientHeight = clientRect.bottom - clientRect.top;

    mSwapChain.Reset();

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferDesc.Width = mClientWidth;
    sd.BufferDesc.Height = mClientHeight;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = mBackBufferFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = window.GetHandle();
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    HRESULT hr = dxgiFactory->CreateSwapChain(
        mCommandQueue.Get(),
        &sd,
        mSwapChain.GetAddressOf()
    );

    if (FAILED(hr)) {
        MessageBox(NULL, L"Failed to create swap chain", L"Error", MB_OK);
        return false;
    }

    return true;
}

void DirectXApp::QueryDescriptorSizes() {
    mRtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCbvSrvUavDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    std::wstring msg = L"Descriptor sizes:\n";
    msg += L"RTV: " + std::to_wstring(mRtvDescriptorSize) + L" bytes\n";
    msg += L"DSV: " + std::to_wstring(mDsvDescriptorSize) + L" bytes\n";
    msg += L"CBV/SRV/UAV: " + std::to_wstring(mCbvSrvUavDescriptorSize) + L" bytes\n";

    MessageBox(NULL, msg.c_str(), L"Step 6: Descriptor Sizes", MB_OK);
}

bool DirectXApp::CreateDescriptorHeaps() {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;

    HRESULT hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap));
    if (FAILED(hr)) {
        MessageBox(NULL, L"Failed to create RTV descriptor heap", L"Error", MB_OK);
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;

    hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap));
    if (FAILED(hr)) {
        MessageBox(NULL, L"Failed to create DSV descriptor heap", L"Error", MB_OK);
        return false;
    }

    MessageBox(NULL, L"Descriptor heaps created successfully", L"Step 7: Descriptor Heaps", MB_OK);
    return true;
}

bool DirectXApp::CreateRenderTargetViews() {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < SwapChainBufferCount; i++) {
        HRESULT hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i]));
        if (FAILED(hr)) {
            MessageBox(NULL, L"Failed to get swap chain buffer", L"Error", MB_OK);
            return false;
        }

        device->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.ptr += mRtvDescriptorSize;
    }

    MessageBox(NULL, L"Render Target Views created", L"Step 8: RTVs", MB_OK);
    return true;
}

bool DirectXApp::CreateDepthStencilBuffer() {
    // Создаем описание ресурса (по скриншоту)
    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mClientWidth;
    depthStencilDesc.Height = mClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = mDepthStencilFormat;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // Описываем очистку (по скриншоту)
    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    // Создаем свойства кучи (замена CD3DX12_HEAP_PROPERTIES)
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    // Создаем ресурс глубины (по скриншоту)
    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(&mDepthStencilBuffer)
    );

    if (FAILED(hr)) {
        MessageBox(NULL, L"Failed to create depth stencil buffer", L"Error", MB_OK);
        return false;
    }

    // Создаем Depth Stencil View (по скриншоту)
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = mDepthStencilFormat;
    dsvDesc.Texture2D.MipSlice = 0;

    device->CreateDepthStencilView(
        mDepthStencilBuffer.Get(),
        &dsvDesc,
        DepthStencilView()
    );

    MessageBox(NULL, L"Depth/Stencil buffer created", L"Step 9: Depth Buffer", MB_OK);
    return true;
}

bool DirectXApp::Initialize() {
    MessageBox(NULL, L"Starting DirectX 12 initialization...", L"Info", MB_OK);

    // Шаг 1: Фабрика DXGI
    if (!CreateDXGIFactory()) return false;

    // Шаг 2: Устройство D3D12
    if (!CreateD3DDevice()) return false;

    // Шаг 3: Командные объекты
    if (!CreateCommandObjects()) return false;

    // Шаг 4: Fence
    if (!CreateFence()) return false;

    // Шаг 5: SwapChain
    if (!CreateSwapChain()) return false;

    // Шаг 6: Размеры дескрипторов
    QueryDescriptorSizes();

    // Шаг 7: Дескрипторные кучи
    if (!CreateDescriptorHeaps()) return false;

    // Шаг 8: Render Target Views
    if (!CreateRenderTargetViews()) return false;

    // Шаг 9: Depth/Stencil Buffer
    if (!CreateDepthStencilBuffer()) return false;

    // Шаг 10: Viewport и Scissor
    CreateViewportAndScissor();

    std::wstring successMsg = L"🎉 DirectX 12 initialization COMPLETE! 🎉\n\n";
    successMsg += L"All 10 steps completed:\n";
    successMsg += L"1. ✓ DXGI Factory\n";
    successMsg += L"2. ✓ D3D12 Device\n";
    successMsg += L"3. ✓ Command Objects\n";
    successMsg += L"4. ✓ Fence\n";
    successMsg += L"5. ✓ SwapChain\n";
    successMsg += L"6. ✓ Descriptor Sizes\n";
    successMsg += L"7. ✓ Descriptor Heaps\n";
    successMsg += L"8. ✓ Render Target Views\n";
    successMsg += L"9. ✓ Depth/Stencil Buffer\n";
    successMsg += L"10. ✓ Viewport & Scissor\n\n";
    successMsg += L"Window: " + std::to_wstring(mClientWidth) + L"x" + std::to_wstring(mClientHeight) + L"\n";
    successMsg += L"Viewport: " + std::to_wstring((int)mScreenViewport.Width) + L"x" + std::to_wstring((int)mScreenViewport.Height) + L"\n";
    successMsg += L"Scissor: " + std::to_wstring(mScissorRect.right) + L"x" + std::to_wstring(mScissorRect.bottom);

    MessageBox(NULL, successMsg.c_str(), L"🎯 DirectX 12 Lab - SUCCESS", MB_OK);
    return true;
}

void DirectXApp::CreateViewportAndScissor() {
    // Создаем Viewport (по скриншоту)
    mScreenViewport.TopLeftX = 0.0f;
    mScreenViewport.TopLeftY = 0.0f;
    mScreenViewport.Width = static_cast<float>(mClientWidth);
    mScreenViewport.Height = static_cast<float>(mClientHeight);
    mScreenViewport.MinDepth = 0.0f;
    mScreenViewport.MaxDepth = 1.0f;

    // Создаем Scissor (по скриншоту)
    mScissorRect = { 0, 0, mClientWidth / 2, mClientHeight / 2 };

    MessageBox(NULL, L"Viewport and Scissor created", L"Step 10: Viewport/Scissor", MB_OK);
}

void DirectXApp::SetViewportAndScissor() {
    // Устанавливаем Viewport в командный список (по скриншоту)
    mCommandList->RSSetViewports(1, &mScreenViewport);

    // Устанавливаем Scissor в командный список (по скриншоту)
    mCommandList->RSSetScissorRects(1, &mScissorRect);
}