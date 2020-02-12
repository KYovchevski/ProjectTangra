#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
struct ServiceLocator;

// Base class for all resource wrappers to give them common functionality such as naming

class RenderResource
{
public:

    RenderResource() = default;
    RenderResource(Microsoft::WRL::ComPtr<ID3D12Resource> a_defaultBuffer) : m_DefaultBuffer(a_defaultBuffer) {}

    void SetName(std::wstring a_NewName);

    ~RenderResource();
protected:

    Microsoft::WRL::ComPtr<ID3D12Resource> m_DefaultBuffer;
};

