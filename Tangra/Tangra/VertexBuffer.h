#pragma once


#include "d3dx12.h"
#include "wrl.h"
#include <vector>
#include "Application.h"
#include "Device.h"
#include "GraphicsCommandList.h"
#include "RenderResource.h"
#include "ServiceLocator.h"

class VertexBuffer
    : public RenderResource
{
public:

    template <typename T>
    VertexBuffer(ServiceLocator& a_ServiceLocator, std::vector<T> a_Vertices, GraphicsCommandList& a_CommandList, D3D12_RESOURCE_FLAGS a_Flags = D3D12_RESOURCE_FLAG_NONE);

    UINT GetNumVertices() const;

    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();

private:

    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
    UINT m_NumVertices;

    bool m_IsValid;
};

template <typename T>
VertexBuffer::VertexBuffer(ServiceLocator& a_ServiceLocator, std::vector<T> a_Vertices, GraphicsCommandList& a_CommandList, D3D12_RESOURCE_FLAGS a_Flags)
    : RenderResource(a_ServiceLocator)
{
    auto device = m_Services.m_Device.get();
    // describe the vertex buffer to be created
    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(a_Vertices.size() * sizeof(T), a_Flags);

    // create a resource for both the default and upload buffer that is to be used
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    device->GetDeviceObject()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_DefaultBuffer));
    heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    device->GetDeviceObject()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_UploadBuffer));

    // describe the subresource and update it
    D3D12_SUBRESOURCE_DATA subresourceData;
    subresourceData.pData = &a_Vertices[0];
    subresourceData.RowPitch = a_Vertices.size() * sizeof(T);
    // the subresource is used for a vertex buffer, so the row pitch and the slice pitch are the same
    subresourceData.SlicePitch = subresourceData.RowPitch;

    UpdateSubresources(a_CommandList.GetCommandListPtr().Get(), m_DefaultBuffer.Get(), m_UploadBuffer.Get(),
        0, 0, 1, &subresourceData);

    // transition the vertex buffer resource into a read state
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_DefaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    a_CommandList.ResourceBarrier(barrier);

    // create the vertex buffer view
    m_VertexBufferView.BufferLocation = m_DefaultBuffer->GetGPUVirtualAddress();
    m_VertexBufferView.SizeInBytes = static_cast<UINT>(a_Vertices.size()) * sizeof(T);
    m_VertexBufferView.StrideInBytes = sizeof(T);

    m_NumVertices = static_cast<UINT>(a_Vertices.size());
    m_IsValid = true;
}

