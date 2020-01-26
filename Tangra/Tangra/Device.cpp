#include "Device.h"
#include "Helpers.h"
#include <iostream>

using namespace Microsoft::WRL;

Device::Device(Microsoft::WRL::ComPtr<IDXGIAdapter4> a_GraphicsAdapter) :
    m_NumSRVHeapEntries(0)
{
#ifdef _DEBUG
    // if the application is ran in debug, enable the debug layer
    ComPtr<ID3D12Debug3> debugInterface;

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
    {
        std::cout << "Application running in debug mode, enabling D3D12 debug layer." << std::endl;
        debugInterface->EnableDebugLayer();
    }
    else
    {
        std::cout << "Application running in debug mode, but D3D12 debug layer could not be enabled." << std::endl;
    }

#endif

    std::cout << "Creating D3D12 Device" << std::endl;
    ComPtr<ID3D12Device> tempDevice;
    ThrowIfFailed(D3D12CreateDevice(a_GraphicsAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&tempDevice)));

    ThrowIfFailed(tempDevice.As(&m_D3D12Device));

    // query the descriptor sizes once only and save them for future use
    m_CBVDescriptorSize = m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_RTVDescriptorSize = m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_DSVDescriptorSize = m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


    m_SRVHeapCapacity = 128;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.NodeMask = 0;
    heapDesc.NumDescriptors = m_SRVHeapCapacity;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    ThrowIfFailed(m_D3D12Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_SRVHeap)));
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Device::AddSRV(ComPtr<ID3D12Resource> a_SRVResource)
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_SRVHeap->GetCPUDescriptorHandleForHeapStart());

    if (m_NumSRVHeapEntries == m_SRVHeapCapacity)
    {
        std::cout << "ERROR: SRV Descriptor heap full, cannot allocated texture" << std::endl;
        abort();
    }

    rHandle.Offset(m_NumSRVHeapEntries, m_CBVDescriptorSize);

    m_D3D12Device->CreateShaderResourceView(a_SRVResource.Get(), nullptr, rHandle);
    

    auto gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_SRVHeap->GetGPUDescriptorHandleForHeapStart()).Offset(m_NumSRVHeapEntries, m_CBVDescriptorSize);

    ++m_NumSRVHeapEntries;
    return gpuHandle;
}

Microsoft::WRL::ComPtr<ID3D12Device2> Device::GetDeviceObject()
{
    return m_D3D12Device;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Device::GetSRVHeap()
{
    return m_SRVHeap;
}

UINT Device::GetDescriptorHandleSize(D3D12_DESCRIPTOR_HEAP_TYPE a_Type) const
{
    switch (a_Type)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:

        return m_RTVDescriptorSize;
    case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:

        return m_DSVDescriptorSize;
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:

        return m_CBVDescriptorSize;
    default:
        std::cout << "ERROR: Cannot get descriptor size of specified type." << std::endl;
        return 0;
    }
}
