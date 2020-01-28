#include "SwapChain.h"
#include "Helpers.h"
#include "Application.h"
#include "CommandQueue.h"
#include "Device.h"
#include "GraphicsCommandList.h"

#include "d3dx12.h"

using namespace Microsoft::WRL;

SwapChain::SwapChain(HWND a_HWND, uint8_t a_NumBackBuffers, CommandQueue& a_CommandQueue, DXGI_FORMAT a_BackBufferFormat)
{
    m_BackBufferFormat = a_BackBufferFormat;
    m_NumBackBuffers = a_NumBackBuffers;
    m_CurrentBackBuffer = 0;
    m_DepthStencilFormat = DXGI_FORMAT_UNKNOWN;

    m_CommandQueue = &a_CommandQueue;

    Application* app = Application::Get();
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    {
        swapChainDesc.BufferDesc.Width = app->GetScreenWidth();
        swapChainDesc.BufferDesc.Height = app->GetScreenHeight();
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferDesc.Format = a_BackBufferFormat;
        swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;

        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        swapChainDesc.Windowed = true;

        swapChainDesc.BufferCount = a_NumBackBuffers;
        swapChainDesc.OutputWindow = a_HWND;

        ThrowIfFailed(app->GetDXGIFactory()->CreateSwapChain(a_CommandQueue.GetCommandQueueObject().Get(), &swapChainDesc, &m_DXGISwapChain));
    }
}

DXGI_FORMAT SwapChain::GetBackBufferFormat() const
{
    return m_BackBufferFormat;
}

uint8_t SwapChain::GetBufferCount() const
{
    return m_NumBackBuffers;
}


void SwapChain::CreateDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};

    // fill out desc struct for RTV heap
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    rtvHeapDesc.NumDescriptors = m_NumBackBuffers;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    // fill out desc struct for DSV heap
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    auto d3d12Device = Application::Get()->GetDevice()->GetDeviceObject();
    d3d12Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVDescriptorHeap));
    d3d12Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVDescriptorHeap));
}

void SwapChain::CreateRenderTargets()
{
    auto d3d12Device = Application::Get()->GetDevice()->GetDeviceObject();

    // handle to the beginning of the RTV descriptor heap
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (size_t i = 0; i < m_NumBackBuffers; i++)
    {
        // create a new resource interface and allocate one of the swap chain's buffers in it
        
        ComPtr<ID3D12Resource> buffer;// = m_BackBuffers.back();

        m_DXGISwapChain->GetBuffer(static_cast<int>(i), IID_PPV_ARGS(&buffer));
        buffer->SetName((std::wstring(L"Render Target #") + std::to_wstring(i)).c_str());

        // create an RTV for the resource
        d3d12Device->CreateRenderTargetView(buffer.Get(), nullptr, rtvHandle);

        // move the descriptor handle to the next descriptor in the heap
        rtvHandle.Offset(1, Application::Get()->GetDevice()->GetDescriptorHandleSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
        m_BackBuffers.push_back(buffer);
    }

}

void SwapChain::CreateDepthStencilBuffer(GraphicsCommandList& a_CommandList, DXGI_FORMAT a_Format)
{
    m_DepthStencilFormat = a_Format;
    auto app = Application::Get();

    D3D12_RESOURCE_DESC dsvResourceDesc = {};
    dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    dsvResourceDesc.Alignment = 0;
    dsvResourceDesc.Width = app->GetScreenWidth();
    dsvResourceDesc.Height = app->GetScreenHeight();
    dsvResourceDesc.DepthOrArraySize = 1;
    dsvResourceDesc.MipLevels = 1;
    dsvResourceDesc.SampleDesc.Count = 1;
    dsvResourceDesc.SampleDesc.Quality = 0;
    dsvResourceDesc.Format = a_Format;
    dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = dsvResourceDesc.Format;
    clearValue.DepthStencil.Depth = 1;
    clearValue.DepthStencil.Stencil = 0;

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(app->GetDevice()->GetDeviceObject()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
        &dsvResourceDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&m_DepthStencilBuffer)));

    m_DepthStencilBuffer->SetName(L"Depth Stencil Buffer");

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    app->GetDevice()->GetDeviceObject()->CreateDepthStencilView(m_DepthStencilBuffer.Get(), nullptr, GetDSVHandle());
    a_CommandList.ResourceBarrier(barrier);
}

void SwapChain::SetClearColor(DirectX::SimpleMath::Color a_NewClearColor)
{
    m_ClearColor = a_NewClearColor;
}

void SwapChain::ClearBackBuffer(GraphicsCommandList& a_CommandList)
{
    auto currentBackBuffer = GetCurrentBackbufferResource();
    auto currentRTV = GetCurrentRTVHandle();

    auto cmdList = a_CommandList.GetCommandListPtr();

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmdList->ResourceBarrier(1, &barrier);

    cmdList->ClearRenderTargetView(currentRTV, m_ClearColor, 0, nullptr);

}

void SwapChain::ClearDSV(GraphicsCommandList& a_CommandList)
{
    a_CommandList.GetCommandListPtr()->ClearDepthStencilView(GetDSVHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void SwapChain::Present()
{
    auto currentBackBuffer = GetCurrentBackbufferResource();
    auto currentRTV = GetCurrentRTVHandle();

    auto cmdList = m_CommandQueue->GetCommandList();

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    cmdList->ResourceBarrier(barrier);

    m_CommandQueue->ExecuteCommandList(*cmdList);

    ThrowIfFailed(m_DXGISwapChain->Present(1, 0));

    m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % m_NumBackBuffers;

    // for now, flush the command queue after each frame to keep it easy
    //m_CommandQueue->Flush();
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetCurrentRTVHandle()
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentBackBuffer,
        Application::Get()->GetDevice()->GetDescriptorHandleSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
}

Microsoft::WRL::ComPtr<ID3D12Resource> SwapChain::GetCurrentBackbufferResource()
{
    return m_BackBuffers[m_CurrentBackBuffer];
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetDSVHandle()
{
    return m_DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

DXGI_FORMAT SwapChain::GetDepthStencilFormat() const
{
    return m_DepthStencilFormat;
}
