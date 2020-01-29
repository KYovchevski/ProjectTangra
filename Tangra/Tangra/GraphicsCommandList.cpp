
#include "GraphicsCommandList.h"
#include "Helpers.h"
#include "Application.h"
#include "Device.h"
#include "PipelineState.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ServiceLocator.h"


GraphicsCommandList::GraphicsCommandList(ServiceLocator& a_ServiceLocator, D3D12_COMMAND_LIST_TYPE a_Type)
    : m_Type(a_Type)
    , m_FenceValue(std::numeric_limits<UINT64>::max())
    , m_Services(a_ServiceLocator)
{
    auto device = m_Services.m_Device->GetDeviceObject();

    ThrowIfFailed(device->CreateCommandAllocator(a_Type, IID_PPV_ARGS(&m_D3D12CommandAllocator)));
    ThrowIfFailed(device->CreateCommandList(0, a_Type, m_D3D12CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_D3D12CommandList)));

}

void GraphicsCommandList::Reset()
{
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
