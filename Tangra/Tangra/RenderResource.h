#pragma once
#include <d3d12.h>
#include <wrl.h>

class RenderResource
{
public:

    RenderResource() = default;

    ~RenderResource();
protected:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_DefaultBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_UploadBuffer;
};

