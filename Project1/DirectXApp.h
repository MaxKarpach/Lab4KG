#pragma once

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <d3dcompiler.h>  // ДЛЯ КОМПИЛЯЦИИ ШЕЙДЕРОВ
#include <string>
#include <vector>
#include "Window.h"
#include "Timer.h"
#include "vertex.h"
#include "UploadBuffer.h"
#include "ObjectConstants.h"
#include <memory>

using Microsoft::WRL::ComPtr;

class DirectXApp {
public:
    DirectXApp(Window& window);
    ~DirectXApp();

    bool Initialize();
    void Shutdown();

    // Основные методы фреймворка
    int Run();
    virtual bool InitializeApp();
    virtual void Update(const Timer& gt);
    virtual void Draw(const Timer& gt);
    virtual void CalculateFrameStats();

    // Управление таймером
    void StopTimer() { mTimer.Stop(); }
    void StartTimer() { mTimer.Start(); }
    bool IsPaused() const { return mAppPaused; }
    Timer& GetTimer() { return mTimer; }

    // Методы для мыши
    virtual void OnMouseDown(WPARAM btnState, int x, int y) {}
    virtual void OnMouseUp(WPARAM btnState, int x, int y) {}
    virtual void OnMouseMove(WPARAM btnState, int x, int y) {}

    // Обработка изменения размера
    virtual void OnResize();

private:
    Window& window;

    // DXGI
    ComPtr<IDXGIFactory4> dxgiFactory;
    ComPtr<IDXGIAdapter1> adapter;

    // D3D12
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    ComPtr<ID3D12Fence> mFence;
    UINT64 mFenceValue = 0;

    // SwapChain
    ComPtr<IDXGISwapChain> mSwapChain;
    static const int SwapChainBufferCount = 2;
    ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
    int mCurrBackBuffer = 0;

    // Дескрипторы
    ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    ComPtr<ID3D12DescriptorHeap> mDsvHeap;
    ComPtr<ID3D12Resource> mDepthStencilBuffer;

    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;
    UINT mCbvSrvUavDescriptorSize = 0;

    // Форматы
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // Размеры
    int mClientWidth = 800;
    int mClientHeight = 600;

    // Viewport и Scissor
    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT mScissorRect;

    // Таймер и состояние
    Timer mTimer;
    bool mAppPaused = false;
    bool mResizing = false;
    int mFrameCount = 0;
    float mTimeElapsed = 0.0f;
    std::wstring mMainWndCaption = L"DirectX 12 Lab";

    // =========== Geometry ===========
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBufferGPU;
    Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBufferUploader;
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBufferGPU;
    Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBufferUploader;
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView;

    // =========== Shaders ===========
    Microsoft::WRL::ComPtr<ID3DBlob> mvsByteCode = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> mpsByteCode = nullptr;

    // =========== Constant Buffer ===========
    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

    // =========== Root Signature и PSO (будем добавлять позже) ===========
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO;

    // Вспомогательные методы инициализации
    bool CreateDXGIFactory();
    bool GetHardwareAdapter();
    bool CreateD3DDevice();
    bool CreateCommandObjects();
    bool CreateFence();
    void FlushCommandQueue();
    bool CreateSwapChain();
    void QueryDescriptorSizes();
    bool CreateDescriptorHeaps();
    bool CreateRenderTargetViews();
    bool CreateDepthStencilBuffer();
    void CreateViewportAndScissor();
    void SetViewportAndScissor();

    // Методы для геометрии и шейдеров
    void BuildInputLayout();
    void BuildVertexBuffer();
    void BuildIndexBuffer();
    void BuildShaders();
    void BuildConstantBuffer();
    void BuildRootSignature();
    void BuildPSO();

    // Методы для доступа к ресурсам
    ID3D12Resource* CurrentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const {
        return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
    }
};