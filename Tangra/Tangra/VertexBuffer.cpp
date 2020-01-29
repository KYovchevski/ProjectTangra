#include "VertexBuffer.h"



UINT VertexBuffer::GetNumVertices() const
{
    return m_NumVertices;
}

D3D12_VERTEX_BUFFER_VIEW VertexBuffer::GetVertexBufferView()
{
    return m_VertexBufferView;
}
