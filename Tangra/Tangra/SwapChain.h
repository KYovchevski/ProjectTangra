#pragma once

#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <cstdint>
#include <vector>

#include "SimpleMath.h"
#include "GraphicsCommandList.h"

class CommandQueue;

struct ServiceLocator;

class SwapChain
{
public:
    SwapChain(ServiceLocator& a_ServiceLocator, HWND a_HWND, uint8_t a_NumBackBuffers,DXGI_FORMAT a_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM);

    void CreateDescriptorHeaps();
    void CreateRenderTargets();
    void CreateDepthStencilBuffer(GraphicsCommandList& a_CommandList, DXGI_FORMAT a_Format = DXGI_FORMAT_D24_UNORM_S8_UINT);

    void SetClearColor(DirectX::SimpleMath::Color a_NewClearColor);

    void ClearBackBuffer(GraphicsCommandList& a_CommandList);
    void ClearDSV(GraphicsCommandList& a_CommandList);

    void Present();

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVHandle();
    Microsoft::WRL::ComPtr<ID3D12Resource> GetCurrentBackbufferResource();
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle();

    DXGI_FORMAT GetDepthStencilFormat() const;
    DXGI_FORMAT GetBackBufferFormat() const;
    uint8_t GetBufferCount() const;
private:
    ServiceLocator& m_Services;

    Microsoft::WRL::ComPtr<IDXGISwapChain> m_DXGISwapChain;
    CommandQueue* m_CommandQueue;

    DXGI_FORMAT m_DepthStencilFormat;
    DXGI_FORMAT m_BackBufferFormat;
    uint8_t m_NumBackBuffers;
    uint8_t m_CurrentBackBuffer;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_BackBuffers;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;

    DirectX::SimpleMath::Color m_ClearColor;

    
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthStencilBuffer;
};

