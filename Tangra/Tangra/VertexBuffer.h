#pragma once


#include "wrl.h"
#include <vector>
//#include "Application.h"
#include "RenderResource.h"


class VertexBuffer
    : public RenderResource
{
public:

    VertexBuffer();
    VertexBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> a_DefaultBuffer, D3D12_VERTEX_BUFFER_VIEW a_VertexBufferView);
    UINT GetNumVertices() const;

    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();

    VertexBuffer& operator=(const VertexBuffer& a_Other);

private:

    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
    UINT m_NumVertices;

    bool m_IsValid;
};

