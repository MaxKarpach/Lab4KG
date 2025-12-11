#pragma once
#include <DirectXMath.h>

struct ObjectConstants
{
    DirectX::XMFLOAT4X4 mWorldViewProj;

    ObjectConstants()
    {
        DirectX::XMStoreFloat4x4(&mWorldViewProj, DirectX::XMMatrixIdentity());
    }
};