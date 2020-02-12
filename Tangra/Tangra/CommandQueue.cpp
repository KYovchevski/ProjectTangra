#include "CommandQueue.h"
#include "Helpers.h"
#include "Application.h"
#include "Device.h"
#include "GraphicsCommandList.h"
#include "ServiceLocator.h"


#include <iostream>

using namespace Microsoft::WRL;

CommandQueue::CommandQueue(ServiceLocator& a_ServiceLocator, D3D12_COMMAND_LIST_TYPE a_Type, D3D12_COMMAND_QUEUE_FLAGS a_Flags)
    : m_Services(a_ServiceLocator)
    , m_Type(a_Type)
{
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
    commandQueueDesc.Type = a_Type;
    commandQueueDesc.Flags = a_Flags;
    commandQueueDesc.NodeMask = 0;
    commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

    ComPtr<ID3D12Device> device = m_Services.m_Device->GetDeviceObject();

    ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_D3D12CommandQueue)));

    m_FenceValue = 0;
    ThrowIfFailed(device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_D3D12Fence)));
}

CommandQueue::~CommandQueue()
{

}

ComPtr<ID3D12CommandQueue> CommandQueue::GetCommandQueueObject()
{
    return m_D3D12CommandQueue;
}

GraphicsCommandList* CommandQueue::GetCommandList()
{
    // Check if any of the used command lists have been finished based on the fence's completed value
    while (!m_UsedCommandLists.empty())
    {
        UINT64 completedValue = m_D3D12Fence->GetCompletedValue();
        // If the command list has been executed completely, reset it and move it to the list of available lists
        if (m_UsedCommandLists.front()->GetFenceValue() <= completedValue)
        {
            GraphicsCommandList* cmdList = m_UsedCommandLists.front();
            m_UsedCommandLists.pop();

            cmdList->Reset();
            m_AvailableCommandLists.push(cmdList);
        }
        else break;
    }

    // If there are no available command lists, create a new one
    if (m_AvailableCommandLists.empty())
    {
        m_CommandLists.emplace_back(std::make_unique<GraphicsCommandList>(m_Services, m_Type));
        m_CommandLists.back()->SetName(std::wstring(L"CommandList ") + std::to_wstring(m_CommandLists.size()));

        m_AvailableCommandLists.push(m_CommandLists.back().get());
    }

    // Get the command list at the front of the queue and return it.
    GraphicsCommandList* cmdList = m_AvailableCommandLists.front();
    m_AvailableCommandLists.pop();
    return cmdList;
}

void CommandQueue::ExecuteCommandList(GraphicsCommandList& a_CommandList)
{
    // Close the command list and execute it
    a_CommandList.Close();

    ID3D12CommandList* cmdLists[] = { a_CommandList.GetCommandListPtr().Get() };

    m_D3D12CommandQueue->ExecuteCommandLists(1, cmdLists);

    // Increment the fence value and use it to signal the command queue
    ++m_FenceValue;
    m_D3D12CommandQueue->Signal(m_D3D12Fence.Get(), m_FenceValue);

    // Set the fence value that the command list is to be associated with and mark it as in-use
    a_CommandList.SetFenceValue(m_FenceValue);
    m_UsedCommandLists.push(&a_CommandList);
}

void CommandQueue::Flush()
{
    // increment the fence value and signal the command queue with it
    // this adds a command at the end of the queue which will signal the fence once it's executed
    ++m_FenceValue;
    ThrowIfFailed(m_D3D12CommandQueue->Signal(m_D3D12Fence.Get(), m_FenceValue));

    // if the fence hasn't been signaled with the fence value, create an event and assign it to the fence. Then wait till the event happens.
    if (m_D3D12Fence->GetCompletedValue() < m_FenceValue)
    {
        HANDLE eventHandle = CreateEventExW(nullptr, false, false, EVENT_ALL_ACCESS);
        m_D3D12Fence->SetEventOnCompletion(m_FenceValue, eventHandle);

        WaitForSingleObject(eventHandle, static_cast<DWORD>(INFINITY));

        CloseHandle(eventHandle);
    }

}
