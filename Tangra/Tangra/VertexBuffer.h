#pragma once


#include "wrl.h"
#include <vector>
#include "RenderResource.h"

// Wrapper around D3D12Resource used as a VertexBuffer
// Use a command list instance to create one.

class VertexBuffer
    : public RenderResource
{
public:

    VertexBuffer();
    // Used by command list class
    VertexBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> a_DefaultBuffer, D3D12_VERTEX_BUFFER_VIEW a_VertexBufferView);
    UINT GetNumVertices() const;

    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();

    VertexBuffer& operator=(const VertexBuffer& a_Other);

private:

    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
    UINT m_NumVertices;

    bool m_IsValid;
};

