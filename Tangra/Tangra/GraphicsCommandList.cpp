
#include "GraphicsCommandList.h"
#include "Helpers.h"
#include "Application.h"
#include "Device.h"
#include "PipelineState.h"

GraphicsCommandList::GraphicsCommandList(D3D12_COMMAND_LIST_TYPE a_Type)
    : m_Type(a_Type)
{
    auto device = Application::Get()->GetDevice()->GetDeviceObject();

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

void GraphicsCommandList::Draw(UINT a_VertexCount, UINT a_InstanceCount, UINT a_StartVertexLoc, UINT a_StartInstanceLoc)
{
    m_D3D12CommandList->DrawInstanced(a_VertexCount, a_InstanceCount, a_StartVertexLoc, a_StartInstanceLoc);
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GraphicsCommandList::GetCommandListPtr()
{
    return m_D3D12CommandList;
}
