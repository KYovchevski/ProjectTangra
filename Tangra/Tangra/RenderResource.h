#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
struct ServiceLocator;

class RenderResource
{
public:

    RenderResource() = default;
    RenderResource(Microsoft::WRL::ComPtr<ID3D12Resource> a_defaultBuffer) : m_DefaultBuffer(a_defaultBuffer) {}

    void SetName(std::wstring a_NewName);

    ~RenderResource();
protected:

    Microsoft::WRL::ComPtr<ID3D12Resource> m_DefaultBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_UploadBuffer;
};

