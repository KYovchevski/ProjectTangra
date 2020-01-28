#pragma once
#include "RenderResource.h"
#include "Application.h"
#include "GraphicsCommandList.h"
#include <vector>
#include "iostream"

#include "d3dx12.h"
class GraphicsCommandList;

class IndexBuffer :
    public RenderResource
{
public:
    IndexBuffer();

    template <typename T>
    IndexBuffer(std::vector<T> a_Indices, GraphicsCommandList& a_CommandList, D3D12_RESOURCE_FLAGS a_Flags = D3D12_RESOURCE_FLAG_NONE);

    UINT GetNumIndices() const;

    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView();

private:

    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
    UINT m_NumIndices;

    bool m_IsValid;
};

template <typename T>
IndexBuffer::IndexBuffer(std::vector<T> a_Indices, GraphicsCommandList& a_CommandList, D3D12_RESOURCE_FLAGS a_Flags)
{
    m_NumIndices = static_cast<UINT>(a_Indices.size());

    DXGI_FORMAT format;
    // determine the format based on the size of the type
    switch (sizeof(T))
    {
    case 1:
        format = DXGI_FORMAT_R8_UINT;
        break;
    case 2:
        format = DXGI_FORMAT_R16_UINT;
        break;
    case 4:
        format = DXGI_FORMAT_R32_UINT;
        break;
    default:
        std::cout << "ERROR: Index buffer initialized with unknown index type of size " << sizeof(T) << std::endl;
        format = DXGI_FORMAT_UNKNOWN;
        break;
    }

    auto device = Application::Get()->GetDevice()->GetDeviceObject();

    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(T) * a_Indices.size(), a_Flags);


    // create upload and default resources
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_DefaultBuffer));
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_UploadBuffer));

    D3D12_SUBRESOURCE_DATA subresourceData;
    subresourceData.RowPitch = sizeof(T) * a_Indices.size();
    subresourceData.SlicePitch = subresourceData.RowPitch;
    subresourceData.pData = &a_Indices[0];

    UpdateSubresources(a_CommandList.GetCommandListPtr().Get(), m_DefaultBuffer.Get(), m_UploadBuffer.Get(),
        0, 0, 1, &subresourceData);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_DefaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

    a_CommandList.ResourceBarrier(barrier);

    m_IndexBufferView.Format = format;
    m_IndexBufferView.BufferLocation = m_DefaultBuffer->GetGPUVirtualAddress();
    m_IndexBufferView.SizeInBytes = sizeof(T) * m_NumIndices;

}

