#pragma once

#include "RenderResource.h"
#include "d3dx12.h"

#include <string>

class GraphicsCommandList;


class Texture
    : public RenderResource
{
public:
    Texture();

    Texture(Microsoft::WRL::ComPtr<ID3D12Resource> a_Buffer, CD3DX12_GPU_DESCRIPTOR_HANDLE a_DescriptorHandle);


    CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle() const;

private:

    CD3DX12_GPU_DESCRIPTOR_HANDLE m_DescriptorHandle;
};

