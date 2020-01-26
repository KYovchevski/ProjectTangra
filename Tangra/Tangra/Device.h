#pragma once
#include "wrl.h"
#include "dxgi1_6.h"
#include "d3dx12.h"

class Device
{
public:

    Device(Microsoft::WRL::ComPtr<IDXGIAdapter4> a_GraphicsAdapter);

    CD3DX12_GPU_DESCRIPTOR_HANDLE AddSRV(Microsoft::WRL::ComPtr<ID3D12Resource> a_SRVResource);


    Microsoft::WRL::ComPtr<ID3D12Device2> GetDeviceObject();

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetSRVHeap();



    UINT GetDescriptorHandleSize(D3D12_DESCRIPTOR_HEAP_TYPE a_Type) const;
private:
    Microsoft::WRL::ComPtr<ID3D12Device2> m_D3D12Device;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SRVHeap;
    UINT m_SRVHeapCapacity;
    UINT m_NumSRVHeapEntries;


    UINT m_RTVDescriptorSize;
    UINT m_DSVDescriptorSize;
    UINT m_CBVDescriptorSize;
};

