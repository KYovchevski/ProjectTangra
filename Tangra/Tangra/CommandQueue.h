#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <cstdint>
#include <memory>
#include <vector>
#include <queue>

class GraphicsCommandList;

struct ServiceLocator;
// Wrapper around ID3D12CommandQueue responsible for GPU synchronization and handling command lists.
class CommandQueue
{
public:
    CommandQueue(ServiceLocator& a_ServiceLocator, D3D12_COMMAND_LIST_TYPE a_Type, D3D12_COMMAND_QUEUE_FLAGS a_Flags = D3D12_COMMAND_QUEUE_FLAG_NONE);
    ~CommandQueue();

    // Gets the command queue COM object
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueueObject();

    // Returns a pointer to an unused command list. Also checks if any previously used command lists have finished execution.
    GraphicsCommandList* GetCommandList();

    // Execute the command list and signal using the fence to know when its execution is finished. Also closes the command list before executing it.
    void ExecuteCommandList(GraphicsCommandList& a_CommandList);

    // Flush all command lists that are currently being executed.
    void Flush();
private:
    ServiceLocator& m_Services;
    
    D3D12_COMMAND_LIST_TYPE m_Type;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_D3D12CommandQueue;

    // To keep it simple, the command queue is responsible entirely for the fence
    uint64_t m_FenceValue;
    Microsoft::WRL::ComPtr<ID3D12Fence1> m_D3D12Fence;

    // List of all the command lists created by the command queue
    std::vector<std::unique_ptr<GraphicsCommandList>> m_CommandLists;

    // List of command lists which have finished execution and are available
    std::queue<GraphicsCommandList*> m_AvailableCommandLists;
    // List of command lists which are currently being executed by the GPU
    std::queue<GraphicsCommandList*> m_UsedCommandLists;

};

