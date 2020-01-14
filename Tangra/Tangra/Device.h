#pragma once
#include "wrl.h"
#include "dxgi1_6.h"
#include "d3d12.h"

class Device
{
public:

    Device(Microsoft::WRL::ComPtr<IDXGIAdapter4> a_GraphicsAdapter);


    Microsoft::WRL::ComPtr<ID3D12Device2> GetDeviceObject();

    UINT GetDescriptorHandleSize(D3D12_DESCRIPTOR_HEAP_TYPE a_Type) const;
private:
    Microsoft::WRL::ComPtr<ID3D12Device2> m_D3D12Device;

    UINT m_RTVDescriptorSize;
    UINT m_DSVDescriptorSize;
    UINT m_CBVDescriptorSize;
};

