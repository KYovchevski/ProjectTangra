#pragma once

#include "wrl.h"
#include "d3d12.h"
#include <vector>

class VertexBuffer;
class PipelineState;

class GraphicsCommandList
{
public:

    GraphicsCommandList(D3D12_COMMAND_LIST_TYPE a_Type);

    void Reset();

    void Close();

    void SetRenderTargets(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> a_RTVHandles, bool a_SingleRTVHandle, D3D12_CPU_DESCRIPTOR_HANDLE a_DSVHandle);
    void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY a_PrimitiveTopology);
    void SetVertexBuffer(VertexBuffer& a_Buffer, UINT a_Slot = 0);
    void SetVertexBuffers(std::vector<VertexBuffer> a_Buffers, UINT a_StartSlot = 0);
    void SetPipelineState(PipelineState& a_NewState);
    void SetViewport(D3D12_VIEWPORT& a_Viewport);
    void SetScissorRect(RECT a_Rect);

    template<typename T>
    void SetRoot32BitConstant(UINT a_RootIndex, T& a_Data, UINT a_OffsetInData = 0);

    void Draw(UINT a_VertexCount, UINT a_InstanceCount = 0, UINT a_StartVertexLoc = 0, UINT a_StartInstanceLoc = 0);

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandListPtr();

private:
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_D3D12CommandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_D3D12CommandAllocator;

    const D3D12_COMMAND_LIST_TYPE m_Type;
};

template <typename T>
void GraphicsCommandList::SetRoot32BitConstant(UINT a_RootIndex, T& a_Data, UINT a_OffsetInData)
{
    m_D3D12CommandList->SetGraphicsRoot32BitConstants(a_RootIndex, sizeof(T) / 4, &a_Data, a_OffsetInData);

}

