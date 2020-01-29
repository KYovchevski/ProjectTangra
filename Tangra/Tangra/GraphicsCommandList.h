#pragma once

#include "wrl.h"
#include "d3dx12.h"
#include <vector>
#include <string>

class Texture;
class IndexBuffer;
class VertexBuffer;
class PipelineState;

struct ServiceLocator;

class GraphicsCommandList
{
public:

    GraphicsCommandList(ServiceLocator& a_ServiceLocator, D3D12_COMMAND_LIST_TYPE a_Type);
    ~GraphicsCommandList(){};

    void Reset();
    void Close();

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

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_D3D12CommandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_D3D12CommandAllocator;

    UINT64 m_FenceValue;

    const D3D12_COMMAND_LIST_TYPE m_Type;
};


template <typename T>
void GraphicsCommandList::SetRoot32BitConstant(UINT a_RootIndex, T& a_Data, UINT a_OffsetInData)
{
    m_D3D12CommandList->SetGraphicsRoot32BitConstants(a_RootIndex, sizeof(T) / 4, &a_Data, a_OffsetInData);

}

