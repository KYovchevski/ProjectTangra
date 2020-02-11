#include "VertexBuffer.h"


VertexBuffer::VertexBuffer()
    : RenderResource()
{
}

VertexBuffer::VertexBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> a_DefaultBuffer,
    D3D12_VERTEX_BUFFER_VIEW a_VertexBufferView)
    : m_VertexBufferView(a_VertexBufferView)
    , m_NumVertices(a_VertexBufferView.SizeInBytes / a_VertexBufferView.StrideInBytes)
{
    m_DefaultBuffer = a_DefaultBuffer;
}

UINT VertexBuffer::GetNumVertices() const
{
    return m_NumVertices;
}

D3D12_VERTEX_BUFFER_VIEW VertexBuffer::GetVertexBufferView()
{
    return m_VertexBufferView;
}

VertexBuffer& VertexBuffer::operator=(const VertexBuffer& a_Other)
{
    m_DefaultBuffer = a_Other.m_DefaultBuffer;
    m_NumVertices = a_Other.m_NumVertices;
    m_VertexBufferView = a_Other.m_VertexBufferView;

    return *this;
}
