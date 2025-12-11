#pragma once

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <string>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace d3dUtil
{
    // Функция для компиляции шейдеров
    ComPtr<ID3DBlob> CompileShader(
        const std::wstring& filename,
        const D3D_SHADER_MACRO* defines,
        const std::string& entrypoint,
        const std::string& target
    );

    // Функция для загрузки бинарных файлов (если используете .cso)
    ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);

    // Утилита для проверки HRESULT
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw std::runtime_error("DX12 call failed");
        }
    }
}