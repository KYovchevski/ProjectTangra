#pragma once

#include "RenderResource.h"
#include "d3dx12.h"

#include <string>

class GraphicsCommandList;

// Wrapper around a D3D12Resouce that is used as a texture
// Use a command list instance to create a texture

class Texture
    : public RenderResource
{
public:
    Texture();

    // Used by command list class
    Texture(Microsoft::WRL::ComPtr<ID3D12Resource> a_Buffer, CD3DX12_GPU_DESCRIPTOR_HANDLE a_DescriptorHandle);


    CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle() const;

private:

    CD3DX12_GPU_DESCRIPTOR_HANDLE m_DescriptorHandle;
};

