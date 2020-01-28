#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <cstdint>
#include <memory>
#include <vector>
#include <queue>

class GraphicsCommandList;

class CommandQueue
{
public:
    CommandQueue(D3D12_COMMAND_LIST_TYPE a_Type, D3D12_COMMAND_QUEUE_FLAGS a_Flags = D3D12_COMMAND_QUEUE_FLAG_NONE);

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueueObject();

    GraphicsCommandList* GetCommandList();

    void ExecuteCommandList(GraphicsCommandList& a_CommandList);

    void Flush();
private:
    
    D3D12_COMMAND_LIST_TYPE m_Type;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_D3D12CommandQueue;

    uint64_t m_FenceValue;
    Microsoft::WRL::ComPtr<ID3D12Fence1> m_D3D12Fence;

    std::vector<std::unique_ptr<GraphicsCommandList>> m_CommandLists;

    std::queue<GraphicsCommandList*> m_AvailableCommandLists;
    std::queue<GraphicsCommandList*> m_UsedCommandLists;

};

