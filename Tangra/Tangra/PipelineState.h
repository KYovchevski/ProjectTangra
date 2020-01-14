#pragma once

#include "wrl.h"
#include "d3dx12.h"

class PipelineState
{
public:

    // struct containing variables which are likely to differ between different PSOs
    struct InitializationData
    {
        D3D12_ROOT_SIGNATURE_FLAGS m_RootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
        std::vector<CD3DX12_ROOT_PARAMETER> m_RootParameters;
        std::vector<CD3DX12_STATIC_SAMPLER_DESC> m_StaticSamplers;
        std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputElements;
        D3D12_PRIMITIVE_TOPOLOGY_TYPE m_PrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        std::wstring m_VertexShaderPath;
        std::wstring m_PixelShaderPath;
        DXGI_FORMAT m_DSVFormat = DXGI_FORMAT_UNKNOWN;
        std::vector<DXGI_FORMAT> m_RTVFormats = {};
        CD3DX12_DEPTH_STENCIL_DESC m_DepthStencil = CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT());
    };

    PipelineState(InitializationData a_InitData);

    // getters for the D3D12 interfaces
    Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPSO();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature();


private:

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE m_RootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT m_InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY m_PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS m_VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS m_PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT m_DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS m_RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL m_depthStencil;
    };

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_D3D12PipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_D3D12RootSignature;

};

