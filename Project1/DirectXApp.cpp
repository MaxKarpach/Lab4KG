#include "DirectXApp.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <string>
#include "d3dUtil.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib") 

// В начало DirectXApp.cpp после #include
struct CD3DX12_RESOURCE_BARRIER_HELPER {
    static D3D12_RESOURCE_BARRIER Transition(
        _In_ ID3D12Resource* pResource,
        D3D12_RESOURCE_STATES stateBefore,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = pResource;
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        barrier.Transition.Subresource = subresource;
        return barrier;
    }
};

DirectXApp::DirectXApp(Window& window) : window(window) {}

DirectXApp::~DirectXApp() {
    Shutdown();
}

// =========== Метод для создания Input Layout ===========
void DirectXApp::BuildInputLayout()
{
    // Для Vertex1 (с цветом) - как на слайде с кубом
    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void DirectXApp::BuildShaders()
{
    mvsByteCode = d3dUtil::CompileShader(
        L"shaders.hlsl",
        nullptr,
        "VS",
        "vs_5_0"
    );

    mpsByteCode = d3dUtil::CompileShader(
        L"shaders.hlsl",
        nullptr,
        "PS",
        "ps_5_0"
    );

    MessageBox(NULL, L"SUCCESS! Shaders compiled", L"Info", MB_OK);
}
// =========== Метод для создания константного буфера ===========
void DirectXApp::BuildConstantBuffer()
{
    // Создаем UploadBuffer для констант (1 элемент)
    mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(
        device.Get(),  // ID3D12Device
        1,             // elementCount (1 объект)
        true           // isConstantBuffer
    );

    // Инициализируем матрицу (единичная матрица)
    ObjectConstants objConstants;

    // Создаем простую матрицу проекции (орфографическую)
    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX proj = DirectX::XMMatrixOrthographicLH(10.0f, 10.0f, 0.1f, 100.0f);
    DirectX::XMMATRIX viewProj = view * proj;

    // Копируем в структуру (транспонируем для HLSL)
    DirectX::XMStoreFloat4x4(&objConstants.mWorldViewProj, XMMatrixTranspose(viewProj));

    // Копируем данные в константный буфер
    mObjectCB->CopyData(0, objConstants);

    MessageBox(NULL, L"Constant buffer created successfully", L"Info", MB_OK);
}

// =========== Метод для создания вершинного буфера ===========
void DirectXApp::BuildVertexBuffer()
{
    // УДАЛИ ЭТИ 3 СТРОКИ (они дублируются ниже):
    // const UINT64 vbByteSize = cubeVertexCount * sizeof(Vertex);
    // D3D12_HEAP_PROPERTIES defaultHeapProps = {};
    // defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    // Размер буфера в байтах (8 вершин * размер Vertex)
    const UINT64 vbByteSize = cubeVertexCount * sizeof(Vertex);

    // 1. Создаем буфер в DEFAULT куче (для GPU)
    D3D12_HEAP_PROPERTIES defaultHeapProps = {};
    defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    defaultHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    defaultHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = vbByteSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&mVertexBufferGPU)
    );

    if (FAILED(hr)) {
        MessageBox(NULL, L"Failed to create vertex buffer (GPU)", L"Error", MB_OK);
        return;
    }

    // 2. Создаем UPLOAD буфер для копирования
    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    hr = device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mVertexBufferUploader)
    );

    if (FAILED(hr)) {
        MessageBox(NULL, L"Failed to create upload buffer", L"Error", MB_OK);
        return;
    }

    // 3. Подготавливаем данные для копирования
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = cubeVertices;
    subResourceData.RowPitch = vbByteSize;
    subResourceData.SlicePitch = vbByteSize;

    // 4. Копируем данные
    mDirectCmdListAlloc->Reset();
    mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr);

    // Барьер: COMMON -> COPY_DEST
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = mVertexBufferGPU.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    mCommandList->ResourceBarrier(1, &barrier);

    // Копируем данные в upload буфер
    BYTE* pData = nullptr;
    hr = mVertexBufferUploader->Map(0, nullptr, reinterpret_cast<void**>(&pData));
    if (SUCCEEDED(hr)) {
        memcpy(pData, cubeVertices, vbByteSize);
        mVertexBufferUploader->Unmap(0, nullptr);
    }

    // Копируем из upload буфера в GPU буфер
    mCommandList->CopyResource(mVertexBufferGPU.Get(), mVertexBufferUploader.Get());

    // Барьер: COPY_DEST -> COMMON
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;

    mCommandList->ResourceBarrier(1, &barrier);

    // Завершаем команды
    mCommandList->Close();

    ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // Ждем завершения копирования
    FlushCommandQueue();

    // 5. Создаем Vertex Buffer View
    mVertexBufferView.BufferLocation = mVertexBufferGPU->GetGPUVirtualAddress();
    mVertexBufferView.SizeInBytes = vbByteSize;
    mVertexBufferView.StrideInBytes = sizeof(Vertex);
}

// =========== Метод для создания индексного буфера ===========
void DirectXApp::BuildIndexBuffer()
{
    // УДАЛИ ЭТИ 3 СТРОКИ (они дублируются ниже):
    // const UINT64 ibByteSize = cubeIndexCount * sizeof(std::uint16_t);
    // D3D12_HEAP_PROPERTIES defaultHeapProps = {};
    // defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    // Размер буфера в байтах (36 индексов * 2 байта) - как в слайде
    const UINT64 ibByteSize = cubeIndexCount * sizeof(std::uint16_t);

    // 1. Создаем буфер в DEFAULT куче (для GPU)
    D3D12_HEAP_PROPERTIES defaultHeapProps = {};
    defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    defaultHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    defaultHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = ibByteSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&mIndexBufferGPU)
    );

    if (FAILED(hr)) {
        MessageBox(NULL, L"Failed to create index buffer (GPU)", L"Error", MB_OK);
        return;
    }

    // 2. Создаем UPLOAD буфер для копирования
    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    hr = device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mIndexBufferUploader)
    );

    if (FAILED(hr)) {
        MessageBox(NULL, L"Failed to create index upload buffer", L"Error", MB_OK);
        return;
    }

    // 3. Подготавливаем данные для копирования
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = cubeIndices;
    subResourceData.RowPitch = ibByteSize;
    subResourceData.SlicePitch = ibByteSize;

    // 4. Копируем данные
    mDirectCmdListAlloc->Reset();
    mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr);

    // Барьер: COMMON -> COPY_DEST
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = mIndexBufferGPU.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    mCommandList->ResourceBarrier(1, &barrier);

    // Копируем данные в upload буфер
    BYTE* pData = nullptr;
    hr = mIndexBufferUploader->Map(0, nullptr, reinterpret_cast<void**>(&pData));
    if (SUCCEEDED(hr)) {
        memcpy(pData, cubeIndices, ibByteSize);
        mIndexBufferUploader->Unmap(0, nullptr);
    }

    // Копируем из upload буфера в GPU буфер
    mCommandList->CopyResource(mIndexBufferGPU.Get(), mIndexBufferUploader.Get());

    // Барьер: COPY_DEST -> COMMON
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;

    mCommandList->ResourceBarrier(1, &barrier);

    // Завершаем команды
    mCommandList->Close();

    ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // Ждем завершения копирования
    FlushCommandQueue();

    // 5. Создаем Index Buffer View (как в слайде)
    mIndexBufferView.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress();
    mIndexBufferView.SizeInBytes = ibByteSize;
    mIndexBufferView.Format = DXGI_FORMAT_R16_UINT;  // 16-битные индексы как в слайде

    MessageBox(NULL, L"Index buffer created successfully", L"Info", MB_OK);
}

// ... остальной код БЕЗ ИЗМЕНЕНИЙ (все методы ниже остаются как есть) ...

void DirectXApp::Shutdown() {
    FlushCommandQueue();

    for (int i = 0; i < SwapChainBufferCount; i++) {
        mSwapChainBuffer[i].Reset();
    }
    mDepthStencilBuffer.Reset();
    mRtvHeap.Reset();
    mDsvHeap.Reset();
    mSwapChain.Reset();

    // Освобождаем буферы
    mVertexBufferGPU.Reset();
    mVertexBufferUploader.Reset();
    mIndexBufferGPU.Reset();      // НОВОЕ
    mIndexBufferUploader.Reset(); // НОВОЕ

    if (mCommandList) {
        mCommandList.Reset();
    }

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
        for (UINT adapterIndex = 0; ; ++adapterIndex) {
            ComPtr<IDXGIAdapter1> currentAdapter;
            HRESULT hr = factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                IID_PPV_ARGS(&currentAdapter));

            if (FAILED(hr)) {
                break;
            }

            DXGI_ADAPTER_DESC1 desc;
            currentAdapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            if (SUCCEEDED(D3D12CreateDevice(currentAdapter.Get(),
                D3D_FEATURE_LEVEL_12_0,
                _uuidof(ID3D12Device), nullptr))) {
                adapter = currentAdapter;
                return true;
            }
        }
    }
    return false;
}

bool DirectXApp::CreateD3DDevice() {
    if (!GetHardwareAdapter()) {
        HRESULT hr = dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
        if (FAILED(hr)) {
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
        &mSwapChain
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

    return true;
}

bool DirectXApp::CreateRenderTargetViews() {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < SwapChainBufferCount; i++) {
        ComPtr<ID3D12Resource> backBuffer;
        HRESULT hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
        if (FAILED(hr)) {
            MessageBox(NULL, L"Failed to get swap chain buffer", L"Error", MB_OK);
            return false;
        }

        mSwapChainBuffer[i] = backBuffer;
        device->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.ptr += mRtvDescriptorSize;
    }

    return true;
}

bool DirectXApp::CreateDepthStencilBuffer() {
    D3D12_RESOURCE_DESC depthStencilDesc = {};
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

    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

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

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = mDepthStencilFormat;
    dsvDesc.Texture2D.MipSlice = 0;

    device->CreateDepthStencilView(
        mDepthStencilBuffer.Get(),
        &dsvDesc,
        DepthStencilView()
    );

    return true;
}

void DirectXApp::CreateViewportAndScissor() {
    mScreenViewport.TopLeftX = 0.0f;
    mScreenViewport.TopLeftY = 0.0f;
    mScreenViewport.Width = static_cast<float>(mClientWidth);
    mScreenViewport.Height = static_cast<float>(mClientHeight);
    mScreenViewport.MinDepth = 0.0f;
    mScreenViewport.MaxDepth = 1.0f;

    mScissorRect = { 0, 0, mClientWidth / 2, mClientHeight / 2 };
}

void DirectXApp::SetViewportAndScissor() {
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);
}

bool DirectXApp::Initialize() {
    MessageBox(NULL, L"Starting DirectX 12 initialization...", L"Info", MB_OK);

    // После BuildConstantBuffer() добавь:
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI,
        (float)mClientWidth / (float)mClientHeight,
        1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);

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

    // Шаг 11: Input Layout
    BuildInputLayout();

    // Шаг 12: Vertex Buffer
    BuildVertexBuffer();

    // Шаг 13: Index Buffer (НОВОЕ)
    BuildIndexBuffer();

    BuildShaders();

    // Шаг 15: Константный буфер (НОВОЕ)
    BuildConstantBuffer();

    mTimer.Reset();
    return true;
}

// ... остальные методы без изменений ...

bool DirectXApp::InitializeApp() {
    return Initialize();
}

ID3D12Resource* DirectXApp::CurrentBackBuffer() const {
    return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXApp::CurrentBackBufferView() const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += mCurrBackBuffer * mRtvDescriptorSize;
    return handle;
}

void DirectXApp::OnResize() {
    // Будет реализовано позже
}

int DirectXApp::Run() {
    MSG msg = { 0 };
    mTimer.Reset();

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            mTimer.Tick();
            if (!mAppPaused) {
                CalculateFrameStats();
                Update(mTimer);
                Draw(mTimer);
            }
            else {
                Sleep(100);
            }
        }
    }
    return (int)msg.wParam;
}

void DirectXApp::CalculateFrameStats() {
    mFrameCount++;
    if ((mTimer.TotalTime() - mTimeElapsed) >= 1.0f) {
        float fps = (float)mFrameCount;
        float mspf = 1000.0f / fps;

        std::wstring fpsStr = std::to_wstring(fps);
        std::wstring mspfStr = std::to_wstring(mspf);

        std::wstring windowText = mMainWndCaption +
            L" fps: " + fpsStr +
            L" mspf: " + mspfStr;

        SetWindowText(window.GetHandle(), windowText.c_str());

        mFrameCount = 0;
        mTimeElapsed += 1.0f;
    }
}

void DirectXApp::Update(const Timer& gt)
{
    // Конвертируем сферические координаты в декартовы
    float x = mRadius * sinf(mPhi) * cosf(mTheta);
    float y = mRadius * sinf(mPhi) * sinf(mTheta);
    float z = mRadius * cosf(mPhi);

    // Создаём матрицу вида
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);

    // Загружаем матрицы
    XMMATRIX world = XMLoadFloat4x4(&mWorld);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);

    // Вычисляем итоговую матрицу (world * view * proj)
    XMMATRIX worldViewProj = world * view * proj;

    // Обновляем константный буфер (транспонируем для HLSL)
    ObjectConstants objConstants;
    XMStoreFloat4x4(&objConstants.mWorldViewProj, XMMatrixTranspose(worldViewProj));

    if (mObjectCB)  // Проверяем, что буфер создан
        mObjectCB->CopyData(0, objConstants);
}

void DirectXApp::Draw(const Timer& gt) {
    // Reuse the memory associated with command recording
    mDirectCmdListAlloc->Reset();

    // A command list can be reset after it has been added to the command queue
    mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr);

    // Indicate a state transition on the resource usage
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER_HELPER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    mCommandList->ResourceBarrier(1, &barrier);

    // Set the viewport and scissor rect
    SetViewportAndScissor();

    // Clear the back buffer and depth buffer
    const float lightSteelBlue[4] = { 0.69f, 0.77f, 0.87f, 1.0f };

    // Получаем дескрипторы
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CurrentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DepthStencilView();

    mCommandList->ClearRenderTargetView(rtvHandle, lightSteelBlue, 0, nullptr);

    mCommandList->ClearDepthStencilView(
        dsvHandle,
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f,
        0,
        0,
        nullptr);

    // Specify the buffers we are going to render to
    mCommandList->OMSetRenderTargets(
        1,
        &rtvHandle,
        true,
        &dsvHandle);

    // TODO: Здесь будет установка вершинного буфера и отрисовка
    // когда будут следующие слайды про PSO, Root Signature и шейдеры

    // Indicate a state transition back to present
    barrier = CD3DX12_RESOURCE_BARRIER_HELPER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);

    mCommandList->ResourceBarrier(1, &barrier);

    // Done recording commands
    mCommandList->Close();

    // Add the command list to the queue for execution
    ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // Swap the back and front buffers
    mSwapChain->Present(0, 0);
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Wait until frame commands are complete
    FlushCommandQueue();
}