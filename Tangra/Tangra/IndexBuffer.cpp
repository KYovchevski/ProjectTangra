#include "IndexBuffer.h"

UINT IndexBuffer::GetNumIndices() const
{
    return m_NumIndices;
}

D3D12_INDEX_BUFFER_VIEW IndexBuffer::GetIndexBufferView()
{
    return m_IndexBufferView;
}
