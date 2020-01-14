#include "VertexBuffer.h"

VertexBuffer::VertexBuffer()
{
    m_IsValid = false;
    m_VertexBufferView = {};
    m_NumVertices = 0;
}

UINT VertexBuffer::GetNumVertices() const
{
    return m_NumVertices;
}

D3D12_VERTEX_BUFFER_VIEW VertexBuffer::GetVertexBufferView()
{
    return m_VertexBufferView;
}
