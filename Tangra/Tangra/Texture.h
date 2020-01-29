#pragma once

#include "RenderResource.h"
#include "d3dx12.h"

#include <string>

class GraphicsCommandList;


class Texture
    : public RenderResource
{
public:
    Texture();

    Texture(std::wstring& a_FilePath, GraphicsCommandList& a_CommandList, ServiceLocator& a_ServiceLocator);

    CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle() const;

private:

    CD3DX12_GPU_DESCRIPTOR_HANDLE m_DescriptorHandle;
};

