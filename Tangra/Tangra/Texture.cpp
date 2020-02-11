#include "Texture.h"
#include "Application.h"
#include "Device.h"
#include "GraphicsCommandList.h"
#include "ServiceLocator.h"

#include "DirectXTex.h"
#include "d3dx12.h"

using namespace DirectX;

Texture::Texture()
{
}

Texture::Texture(Microsoft::WRL::ComPtr<ID3D12Resource> a_Buffer, CD3DX12_GPU_DESCRIPTOR_HANDLE a_DescriptorHandle)
    : RenderResource(a_Buffer)
    , m_DescriptorHandle(a_DescriptorHandle)
{
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Texture::GetGPUDescriptorHandle() const
{
    return m_DescriptorHandle;
}
