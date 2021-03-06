#include "PipelineState.h"
#include "Helpers.h"
#include "Application.h"
#include "Device.h"
#include "ServiceLocator.h"

#include <d3dcompiler.h>

using namespace Microsoft::WRL;

PipelineState::PipelineState(InitializationData a_InitData, ServiceLocator& a_ServiceLocator)
    : m_Services(a_ServiceLocator)
{
    auto device = m_Services.m_Device->GetDeviceObject();

    // Create the root signature description
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    CD3DX12_ROOT_PARAMETER* rootParametersPtr =      !a_InitData.m_RootParameters.empty() ? &a_InitData.m_RootParameters[0] : nullptr;
    CD3DX12_STATIC_SAMPLER_DESC* staticSamplersPtr = !a_InitData.m_StaticSamplers.empty() ? &a_InitData.m_StaticSamplers[0] : nullptr;
    rootSignatureDesc.Init(static_cast<UINT>(a_InitData.m_RootParameters.size()), rootParametersPtr,
        static_cast<UINT>(a_InitData.m_StaticSamplers.size()), staticSamplersPtr, a_InitData.m_RootSignatureFlags);

    // Serialize and create the root signature
    ComPtr<ID3DBlob> rootSignatureSerialized, errorBlob;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureSerialized, &errorBlob));
    ThrowIfFailed(device->CreateRootSignature(0, rootSignatureSerialized->GetBufferPointer(), rootSignatureSerialized->GetBufferSize(), IID_PPV_ARGS(&m_D3D12RootSignature)));

    // Describe the input layout for the pipeline
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
    inputLayoutDesc.NumElements = static_cast<UINT>(a_InitData.m_InputElements.size());
    inputLayoutDesc.pInputElementDescs = &a_InitData.m_InputElements[0];

    // Read shaders from .cso files, maybe make this runtime shader compilation?
    ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(&a_InitData.m_VertexShaderPath[0], &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(&a_InitData.m_PixelShaderPath[0], &pixelShaderBlob));

    // Fill in the RTV formats to 8, appears to be necessary to avoid D3D12 errors
    while (a_InitData.m_RTVFormats.size() < 8)
    {
        a_InitData.m_RTVFormats.push_back(DXGI_FORMAT_UNKNOWN);
    }

    CD3DX12_RT_FORMAT_ARRAY rtvFormats = CD3DX12_RT_FORMAT_ARRAY(&a_InitData.m_RTVFormats[0], static_cast<UINT>(a_InitData.m_RTVFormats.size()));

    // Create the pipeline state stream struct and describe it
    PipelineStateStream stateStream;
    stateStream.m_RootSignature = m_D3D12RootSignature.Get();
    stateStream.m_DSVFormat = a_InitData.m_DSVFormat;
    stateStream.m_InputLayout = inputLayoutDesc;
    stateStream.m_PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    stateStream.m_VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    stateStream.m_PrimitiveTopologyType = a_InitData.m_PrimitiveTopology;
    stateStream.m_RTVFormats = rtvFormats;
    stateStream.m_depthStencil = a_InitData.m_DepthStencil;

    D3D12_PIPELINE_STATE_STREAM_DESC stateStreamDesc;
    stateStreamDesc.SizeInBytes = sizeof(PipelineStateStream);
    stateStreamDesc.pPipelineStateSubobjectStream = &stateStream;

    // create the pipeline from the pipeline state stream
    ThrowIfFailed(device->CreatePipelineState(&stateStreamDesc, IID_PPV_ARGS(&m_D3D12PipelineState)));
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState::GetPSO()
{
    return m_D3D12PipelineState;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipelineState::GetRootSignature()
{
    return m_D3D12RootSignature;
}
