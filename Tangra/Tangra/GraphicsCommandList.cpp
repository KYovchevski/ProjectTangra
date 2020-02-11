
#include "GraphicsCommandList.h"
#include "Helpers.h"
#include "Application.h"
#include "Device.h"
#include "PipelineState.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ServiceLocator.h"
#include <DirectXTex.h>

using namespace DirectX;
using namespace Microsoft::WRL;

GraphicsCommandList::GraphicsCommandList(ServiceLocator& a_ServiceLocator, D3D12_COMMAND_LIST_TYPE a_Type)
    : m_Type(a_Type)
    , m_FenceValue(std::numeric_limits<UINT64>::max())
    , m_Services(a_ServiceLocator)
{
    auto device = m_Services.m_Device->GetDeviceObject();

    ThrowIfFailed(device->CreateCommandAllocator(a_Type, IID_PPV_ARGS(&m_D3D12CommandAllocator)));
    ThrowIfFailed(device->CreateCommandList(0, a_Type, m_D3D12CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_D3D12CommandList)));

}

Texture GraphicsCommandList::CreateTextureFromFilePath(std::wstring& a_FilePath)
{
    ScratchImage scratchImage;
    TexMetadata metaData;
    LoadFromWICFile(a_FilePath.c_str(), WIC_FLAGS_NONE, &metaData, scratchImage);

    DirectX::MakeSRGB(metaData.format);

    auto bufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(metaData.format, static_cast<UINT16>(metaData.width), static_cast<UINT16>(metaData.height), static_cast<UINT16>(metaData.arraySize));

    auto device = m_Services.m_Device->GetDeviceObject();

    ComPtr<ID3D12Resource> defaultBuffer, uploadBuffer;

    {
        CD3DX12_HEAP_PROPERTIES heapProperties;

        heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&defaultBuffer));

        UINT64 intermediateSize = GetRequiredIntermediateSize(defaultBuffer.Get(), 0, 1);

        heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto intermediateDesc = CD3DX12_RESOURCE_DESC::Buffer(intermediateSize);
        device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &intermediateDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
    }

    m_IntermediateBuffers.push_back(uploadBuffer);

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
    UpdateSubresources(m_D3D12CommandList.Get(), defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subresources[0]);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
    ResourceBarrier(barrier);

    CD3DX12_GPU_DESCRIPTOR_HANDLE descriptorHandle = m_Services.m_Device->AddSRV(defaultBuffer);
    
    return Texture(defaultBuffer, descriptorHandle);
}

void GraphicsCommandList::Reset()
{
    for (const auto intermediateBuffer : m_IntermediateBuffers)
    {
        ID3D12Resource* const ptr = intermediateBuffer.Get();
        // i have no clue what i am doing, not even kidding
        m_Services.m_Device->GetDeviceObject()->Evict(1, reinterpret_cast<ID3D12Pageable* const*>(&ptr));
    }

    m_IntermediateBuffers.clear();

    ThrowIfFailed(m_D3D12CommandAllocator->Reset());
    ThrowIfFailed(m_D3D12CommandList->Reset(m_D3D12CommandAllocator.Get(), nullptr));
}

void GraphicsCommandList::Close()
{
    ThrowIfFailed(m_D3D12CommandList->Close());
}

void GraphicsCommandList::ResourceBarrier(D3D12_RESOURCE_BARRIER& a_Barrier)
{
    m_D3D12CommandList->ResourceBarrier(1, &a_Barrier);
}

void GraphicsCommandList::ResourceBarriers(std::vector<D3D12_RESOURCE_BARRIER>& a_Barriers)
{
    m_D3D12CommandList->ResourceBarrier(static_cast<UINT>(a_Barriers.size()), &a_Barriers[0]);
}

void GraphicsCommandList::SetRenderTargets(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> a_RTVHandles, bool a_SingleRTVHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE a_DSVHandle)
{
    m_D3D12CommandList->OMSetRenderTargets(static_cast<UINT>(a_RTVHandles.size()), &a_RTVHandles[0], a_SingleRTVHandle, &a_DSVHandle);
}

void GraphicsCommandList::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY a_PrimitiveTopology)
{
    m_D3D12CommandList->IASetPrimitiveTopology(a_PrimitiveTopology);
}

void GraphicsCommandList::SetVertexBuffer(VertexBuffer& a_Buffer,UINT a_Slot)
{
    auto bufferView = a_Buffer.GetVertexBufferView();
    m_D3D12CommandList->IASetVertexBuffers(a_Slot, 1, &bufferView);
}

void GraphicsCommandList::SetVertexBuffers(std::vector<VertexBuffer> a_Buffers, UINT a_StartSlot)
{
    std::vector<D3D12_VERTEX_BUFFER_VIEW> bufferViews;
    for (VertexBuffer buffer : a_Buffers)
    {
        bufferViews.push_back(a_Buffers[0].GetVertexBufferView());
    }
    m_D3D12CommandList->IASetVertexBuffers(a_StartSlot, static_cast<UINT>(bufferViews.size()), &bufferViews[0]);
}

void GraphicsCommandList::SetIndexBuffer(IndexBuffer& a_IndexBuffer)
{
    auto bufferView = a_IndexBuffer.GetIndexBufferView();
    m_D3D12CommandList->IASetIndexBuffer(&bufferView);
}

void GraphicsCommandList::SetDescriptorHeap(ID3D12DescriptorHeap* a_DescriptorHeap)
{
    m_D3D12CommandList->SetDescriptorHeaps(1, &a_DescriptorHeap);
}

void GraphicsCommandList::SetDescriptorHeaps(std::vector<ID3D12DescriptorHeap*> a_DescriptorHeaps)
{
    m_D3D12CommandList->SetDescriptorHeaps(static_cast<UINT>(a_DescriptorHeaps.size()), &a_DescriptorHeaps[0]);
}

void GraphicsCommandList::SetPipelineState(PipelineState& a_NewState)
{
    m_D3D12CommandList->SetPipelineState(a_NewState.GetPSO().Get());
    m_D3D12CommandList->SetGraphicsRootSignature(a_NewState.GetRootSignature().Get());
}

void GraphicsCommandList::SetViewport(D3D12_VIEWPORT& a_Viewport)
{
    m_D3D12CommandList->RSSetViewports(1, &a_Viewport);
}

void GraphicsCommandList::SetScissorRect(RECT a_Rect)
{
    m_D3D12CommandList->RSSetScissorRects(1, &a_Rect);
}

void GraphicsCommandList::SetTexture(UINT a_RootSignatureIndex, Texture& a_Texture)
{
    m_D3D12CommandList->SetGraphicsRootDescriptorTable(a_RootSignatureIndex, a_Texture.GetGPUDescriptorHandle());
}

void GraphicsCommandList::Draw(UINT a_VertexCount, UINT a_InstanceCount, UINT a_StartVertexLoc, UINT a_StartInstanceLoc)
{
    m_D3D12CommandList->DrawInstanced(a_VertexCount, a_InstanceCount, a_StartVertexLoc, a_StartInstanceLoc);
}

void GraphicsCommandList::DrawIndexed(UINT a_IndexCount, UINT a_InstanceCount, UINT a_StarIndexLoc, UINT a_BaseVertexLoc,
    UINT a_StartInstanceLoc)
{
    m_D3D12CommandList->DrawIndexedInstanced(a_IndexCount, a_InstanceCount, a_StarIndexLoc, a_BaseVertexLoc, a_StartInstanceLoc);
}

void GraphicsCommandList::SetFenceValue(UINT64 a_NewFenceValue)
{
    m_FenceValue = a_NewFenceValue;
}

void GraphicsCommandList::SetName(std::wstring a_Name)
{
    m_D3D12CommandAllocator->SetName((a_Name + L" Allocator").c_str());
    m_D3D12CommandList->SetName(a_Name.c_str());
}

UINT64 GraphicsCommandList::GetFenceValue() const
{
    return m_FenceValue;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GraphicsCommandList::GetCommandListPtr()
{
    return m_D3D12CommandList;
}
