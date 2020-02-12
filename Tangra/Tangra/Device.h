#pragma once
#include "wrl.h"
#include "dxgi1_6.h"
#include "d3dx12.h"

class CommandQueue;

struct ServiceLocator;

class Device
{
public:

    Device(Microsoft::WRL::ComPtr<IDXGIAdapter4> a_GraphicsAdapter, ServiceLocator& a_ServiceLocator);

    // Initialization is deferred to ensure command queues can use the service locator without resulting in nullptr references
    void Initialize();

    // Currently, the device owns the descriptor heap which holds SRVs. Subject to change.
    CD3DX12_GPU_DESCRIPTOR_HANDLE AddSRV(Microsoft::WRL::ComPtr<ID3D12Resource> a_SRVResource);

    Microsoft::WRL::ComPtr<ID3D12Device2> GetDeviceObject();

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetSRVHeap();

    // Returns pointer to the command queue of the requested type
    CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE a_Type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    // Returns the size of the descriptor of the requested type
    UINT GetDescriptorHandleSize(D3D12_DESCRIPTOR_HEAP_TYPE a_Type) const;
private:
    ServiceLocator& m_Services;

    Microsoft::WRL::ComPtr<ID3D12Device2> m_D3D12Device;

    // Device owns the SRV heap currently. Subject to change.
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SRVHeap;

    std::unique_ptr<CommandQueue> m_DirectCommandQueue;
    std::unique_ptr<CommandQueue> m_CopyCommandQueue;
    std::unique_ptr<CommandQueue> m_ComputeCommandQueue;

    UINT m_SRVHeapCapacity;
    UINT m_NumSRVHeapEntries;

    UINT m_RTVDescriptorSize;
    UINT m_DSVDescriptorSize;
    UINT m_CBVDescriptorSize;
};

