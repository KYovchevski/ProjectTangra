#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
struct ServiceLocator;

class RenderResource
{
public:

    RenderResource() = default;
    RenderResource(ServiceLocator& a_ServiceLocator) : m_Services(a_ServiceLocator) {}

    void SetName(std::wstring a_NewName);

    ~RenderResource();
protected:
    ServiceLocator& m_Services;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_DefaultBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_UploadBuffer;
};

