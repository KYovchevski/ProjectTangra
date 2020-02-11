#include "IndexBuffer.h"

IndexBuffer::IndexBuffer()
    : RenderResource()
{
}

UINT IndexBuffer::GetNumIndices() const
{
    return m_NumIndices;
}

D3D12_INDEX_BUFFER_VIEW IndexBuffer::GetIndexBufferView()
{
    return m_IndexBufferView;
}

IndexBuffer& IndexBuffer::operator=(const IndexBuffer& a_Other)
{
    m_NumIndices = a_Other.m_NumIndices;
    m_DefaultBuffer = a_Other.m_DefaultBuffer;
    m_IndexBufferView = a_Other.m_IndexBufferView;

    return *this;
}
