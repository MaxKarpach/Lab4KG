#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <vector>

using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT4 Color;

    Vertex() = default;

    Vertex(float x, float y, float z, float r, float g, float b, float a)
        : Position(x, y, z), Color(r, g, b, a) {
    }
};

inline std::vector<D3D12_INPUT_ELEMENT_DESC> GetVertexInputLayout()
{
    return {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
}