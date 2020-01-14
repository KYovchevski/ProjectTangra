#include "CommandQueue.h"
#include "Helpers.h"
#include "Application.h"
#include "Device.h"
#include "GraphicsCommandList.h"

using namespace Microsoft::WRL;

CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE a_Type, D3D12_COMMAND_QUEUE_FLAGS a_Flags)
{
    D3D12_COMMAND_QUEUE_DESC directCommandQueueDesc;
    directCommandQueueDesc.Type = a_Type;
    directCommandQueueDesc.Flags = a_Flags;
    directCommandQueueDesc.NodeMask = 0;
    directCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

    ComPtr<ID3D12Device> device = Application::Get()->GetDevice()->GetDeviceObject();

    ThrowIfFailed(device->CreateCommandQueue(&directCommandQueueDesc, IID_PPV_ARGS(&m_D3D12CommandQueue)));

    m_CommandList = std::make_unique<GraphicsCommandList>(a_Type);

    m_FenceValue = 0;
    ThrowIfFailed(device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_D3D12Fence)));
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue::GetCommandQueueObject()
{
    return m_D3D12CommandQueue;
}

GraphicsCommandList* CommandQueue::GetCommandList()
{
    return m_CommandList.get();
}

void CommandQueue::ExecuteCommandList(GraphicsCommandList& a_CommandList)
{

    a_CommandList.Close();

    ID3D12CommandList* cmdLists[] = { a_CommandList.GetCommandListPtr().Get() };

    m_D3D12CommandQueue->ExecuteCommandLists(1, cmdLists);
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
