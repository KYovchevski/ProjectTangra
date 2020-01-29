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

    void Initialize();

    CD3DX12_GPU_DESCRIPTOR_HANDLE AddSRV(Microsoft::WRL::ComPtr<ID3D12Resource> a_SRVResource);

    Microsoft::WRL::ComPtr<ID3D12Device2> GetDeviceObject();

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetSRVHeap();

    CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE a_Type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    UINT GetDescriptorHandleSize(D3D12_DESCRIPTOR_HEAP_TYPE a_Type) const;
private:
    ServiceLocator& m_Services;

    Microsoft::WRL::ComPtr<ID3D12Device2> m_D3D12Device;

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

