#include "Texture.h"
#include "Application.h"
#include "Device.h"
#include "GraphicsCommandList.h"

#include "DirectXTex.h"
#include "d3dx12.h"

using namespace DirectX;

Texture::Texture(std::wstring& a_FilePath, GraphicsCommandList& a_CommandList)
{
    ScratchImage scratchImage;
    TexMetadata metaData;
    LoadFromWICFile(a_FilePath.c_str(), WIC_FLAGS_NONE, &metaData, scratchImage);

    MakeSRGB(metaData.format);

    auto bufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(metaData.format, static_cast<UINT16>(metaData.width), static_cast<UINT16>(metaData.height), static_cast<UINT16>(metaData.arraySize));

    auto device = Application::Get()->GetDevice()->GetDeviceObject();

    {
        CD3DX12_HEAP_PROPERTIES heapProperties;

        heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_DefaultBuffer));

        UINT64 intermediateSize = GetRequiredIntermediateSize(m_DefaultBuffer.Get(), 0, 1);

        heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto intermediateDesc = CD3DX12_RESOURCE_DESC::Buffer(intermediateSize);
        device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &intermediateDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_UploadBuffer));
    }

    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    const Image* images = scratchImage.GetImages();
    for (size_t i = 0; i < scratchImage.GetImageCount(); i++)
    {
        D3D12_SUBRESOURCE_DATA subres;
        subres.RowPitch = images[i].rowPitch;
        subres.SlicePitch = images[i].slicePitch;
        subres.pData = images[i].pixels;

        subresources.push_back(subres);
    }
    UpdateSubresources(a_CommandList.GetCommandListPtr().Get(), m_DefaultBuffer.Get(), m_UploadBuffer.Get(), 0, 0, 1, &subresources[0]);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_DefaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
    a_CommandList.GetCommandListPtr()->ResourceBarrier(1, &barrier);

    m_DescriptorHandle = Application::Get()->GetDevice()->AddSRV(m_DefaultBuffer);
    
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Texture::GetGPUDescriptorHandle() const
{
    return m_DescriptorHandle;
}
