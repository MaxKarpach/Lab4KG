#pragma once

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <string>
#include "Window.h"

using Microsoft::WRL::ComPtr;

class DirectXApp {
public:
    DirectXApp(Window& window);
    ~DirectXApp();

    bool Initialize();
    void Shutdown();

private:
    Window& window;

    // DXGI
    ComPtr<IDXGIFactory4> dxgiFactory;
    ComPtr<IDXGIAdapter1> adapter;

    // D3D12
    ComPtr<ID3D12Device> device;

    // Команды DX12
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;

    // Fence для синхронизации
    ComPtr<ID3D12Fence> mFence;
    UINT64 mFenceValue = 0;

    // SwapChain и буферы
    ComPtr<IDXGISwapChain> mSwapChain;
    static const int SwapChainBufferCount = 2;
    ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
    int mCurrBackBuffer = 0;

    // Форматы
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // Размеры окна
    int mClientWidth = 800;
    int mClientHeight = 600;

    // Размеры дескрипторов
    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;
    UINT mCbvSrvUavDescriptorSize = 0;

    // Дескрипторные кучи
    ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    ComPtr<ID3D12DescriptorHeap> mDsvHeap;

    // Depth/Stencil buffer
    ComPtr<ID3D12Resource> mDepthStencilBuffer;

    // Вспомогательные методы
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

    // Вспомогательные методы для дескрипторов
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const {
        return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
    }
    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT mScissorRect;

    // Новый метод для шага 10
    void CreateViewportAndScissor();
    void SetViewportAndScissor();
};