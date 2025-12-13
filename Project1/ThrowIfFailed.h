#pragma once
#include <stdexcept>
#include <string>

template<typename T>
inline void ThrowIfFailed(T hr, const char* message = "DirectX operation failed")
{
    if (FAILED(hr))
    {
        throw std::runtime_error(message);
    }
}
