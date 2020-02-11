#pragma once

#include "wrl.h"
#include "d3dx12.h"

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ServiceLocator.h"
#include "Device.h"

#include <vector>
#include <string>

class Texture;
class PipelineState;

struct ServiceLocator;

class GraphicsCommandList
{
public:

    GraphicsCommandList(ServiceLocator& a_ServiceLocator, D3D12_COMMAND_LIST_TYPE a_Type);
    ~GraphicsCommandList(){};

    void Reset();
    void Close();

    template<typename T>
    VertexBuffer CreateVertexBuffer(std::vector<T> a_Vertices, D3D12_RESOURCE_FLAGS a_Flags = D3D12_RESOURCE_FLAG_NONE);

    template<typename T>
    IndexBuffer CreateIndexBuffer(std::vector<T> a_Indices, D3D12_RESOURCE_FLAGS a_Flags = D3D12_RESOURCE_FLAG_NONE);

    Texture CreateTextureFromFilePath(std::wstring& a_FilePath);

    void ResourceBarrier(D3D12_RESOURCE_BARRIER& a_Barrier);
    void ResourceBarriers(std::vector<D3D12_RESOURCE_BARRIER>& a_Barriers);

    void SetPipelineState(PipelineState& a_NewState);

    void SetRenderTargets(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> a_RTVHandles, bool a_SingleRTVHandle, D3D12_CPU_DESCRIPTOR_HANDLE a_DSVHandle);
    void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY a_PrimitiveTopology);
    void SetViewport(D3D12_VIEWPORT& a_Viewport);
    void SetScissorRect(RECT a_Rect);

    void SetVertexBuffer(VertexBuffer& a_Buffer, UINT a_Slot = 0);
    void SetVertexBuffers(std::vector<VertexBuffer> a_Buffers, UINT a_StartSlot = 0);
    void SetIndexBuffer(IndexBuffer& a_IndexBuffer);
    void SetDescriptorHeap(ID3D12DescriptorHeap* a_DescriptorHeap);
    void SetDescriptorHeaps(std::vector<ID3D12DescriptorHeap*> a_DescriptorHeaps);

    template<typename T>
    void SetRoot32BitConstant(UINT a_RootIndex, T& a_Data, UINT a_OffsetInData = 0);
    void SetTexture(UINT a_RootSignatureIndex, Texture& a_Texture);

    void Draw(UINT a_VertexCount, UINT a_InstanceCount = 1, UINT a_StartVertexLoc = 0, UINT a_StartInstanceLoc = 0);
    void DrawIndexed(UINT a_IndexCount, UINT a_InstanceCount = 1, UINT a_StarIndexLoc = 0, UINT a_BaseVertexLoc = 0, UINT a_StartInstanceLoc = 0);

    void SetFenceValue(UINT64 a_NewFenceValue);
    void SetName(std::wstring a_Name);

    UINT64 GetFenceValue() const;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandListPtr();

private:
    ServiceLocator& m_Services;

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_IntermediateBuffers;

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_D3D12CommandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_D3D12CommandAllocator;

    UINT64 m_FenceValue;

    const D3D12_COMMAND_LIST_TYPE m_Type;
};


template <typename T>
VertexBuffer GraphicsCommandList::CreateVertexBuffer(std::vector<T> a_Vertices,
    D3D12_RESOURCE_FLAGS a_Flags)
{
    auto device = m_Services.m_Device.get();
    // describe the vertex buffer to be created
    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(a_Vertices.size() * sizeof(T), a_Flags);

    Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer, uploadBuffer;

    // create a resource for both the default and upload buffer that is to be used
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    device->GetDeviceObject()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&defaultBuffer));
    heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    device->GetDeviceObject()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));

    m_IntermediateBuffers.push_back(uploadBuffer);

    // describe the subresource and update it
    D3D12_SUBRESOURCE_DATA subresourceData;
    subresourceData.pData = &a_Vertices[0];
    subresourceData.RowPitch = a_Vertices.size() * sizeof(T);
    // the subresource is used for a vertex buffer, so the row pitch and the slice pitch are the same
    subresourceData.SlicePitch = subresourceData.RowPitch;

    UpdateSubresources(m_D3D12CommandList.Get(), defaultBuffer.Get(), uploadBuffer.Get(),
        0, 0, 1, &subresourceData);

    // transition the vertex buffer resource into a read state
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    ResourceBarrier(barrier);

    // create the vertex buffer view
    D3D12_VERTEX_BUFFER_VIEW vbView = {};

    vbView.BufferLocation = defaultBuffer->GetGPUVirtualAddress();
    vbView.SizeInBytes = static_cast<UINT>(a_Vertices.size()) * sizeof(T);
    vbView.StrideInBytes = sizeof(T);

    return VertexBuffer(defaultBuffer, vbView);
}

template <typename T>
IndexBuffer GraphicsCommandList::CreateIndexBuffer(std::vector<T> a_Indices, D3D12_RESOURCE_FLAGS a_Flags)
{
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

    auto device = m_Services.m_Device->GetDeviceObject();

    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(T) * a_Indices.size(), a_Flags);

    Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer, uploadBuffer;

    // create upload and default resources
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&defaultBuffer));
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));

    m_IntermediateBuffers.push_back(uploadBuffer);

    D3D12_SUBRESOURCE_DATA subresourceData;
    subresourceData.RowPitch = sizeof(T) * a_Indices.size();
    subresourceData.SlicePitch = subresourceData.RowPitch;
    subresourceData.pData = &a_Indices[0];

    UpdateSubresources(m_D3D12CommandList.Get(), defaultBuffer.Get(), uploadBuffer.Get(),
        0, 0, 1, &subresourceData);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

    ResourceBarrier(barrier);

    unsigned int numIndices = static_cast<UINT>(a_Indices.size());
    D3D12_INDEX_BUFFER_VIEW bufferView;
    bufferView.Format = format;
    bufferView.BufferLocation = defaultBuffer->GetGPUVirtualAddress();
    bufferView.SizeInBytes = sizeof(T) * numIndices;

    return IndexBuffer(defaultBuffer, bufferView, numIndices);
}

template <typename T>
void GraphicsCommandList::SetRoot32BitConstant(UINT a_RootIndex, T& a_Data, UINT a_OffsetInData)
{
    m_D3D12CommandList->SetGraphicsRoot32BitConstants(a_RootIndex, sizeof(T) / 4, &a_Data, a_OffsetInData);

}

