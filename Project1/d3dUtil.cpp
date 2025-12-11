#include "d3dUtil.h"
#include <fstream>
#include <iostream>

namespace d3dUtil
{
    ComPtr<ID3DBlob> CompileShader(
        const std::wstring& filename,
        const D3D_SHADER_MACRO* defines,
        const std::string& entrypoint,
        const std::string& target)
    {
        UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        HRESULT hr = S_OK;

        ComPtr<ID3DBlob> byteCode = nullptr;
        ComPtr<ID3DBlob> errors;

        hr = D3DCompileFromFile(
            filename.c_str(),
            defines,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entrypoint.c_str(),
            target.c_str(),
            compileFlags,
            0,
            &byteCode,
            &errors
        );

        // Вывод ошибок компиляции в Output Window
        if (errors != nullptr)
        {
            OutputDebugStringA((char*)errors->GetBufferPointer());
            // Также можно вывести в консоль
            std::cout << "Shader compile errors:\n"
                << (char*)errors->GetBufferPointer() << std::endl;
        }

        ThrowIfFailed(hr);
        return byteCode;
    }

    ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename)
    {
        std::ifstream fin(filename, std::ios::binary);

        if (!fin.is_open())
        {
            throw std::runtime_error("Failed to open file: " +
                std::string(filename.begin(), filename.end()));
        }

        fin.seekg(0, std::ios_base::end);
        std::ifstream::pos_type size = fin.tellg();
        fin.seekg(0, std::ios_base::beg);

        ComPtr<ID3DBlob> blob;
        ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

        fin.read((char*)blob->GetBufferPointer(), size);
        fin.close();

        return blob;
    }
}