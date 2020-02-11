#pragma once
#include "RenderResource.h"

#include "d3dx12.h"

#include <vector>
#include "iostream"

class GraphicsCommandList;

class IndexBuffer :
    public RenderResource
{
public:

    IndexBuffer();
    IndexBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> a_Buffer, D3D12_INDEX_BUFFER_VIEW a_BufferView, unsigned int a_NumIndices)
        : RenderResource(a_Buffer)
        , m_IndexBufferView(a_BufferView)
        , m_NumIndices(a_NumIndices){}

    UINT GetNumIndices() const;

    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView();

    IndexBuffer& operator=(const IndexBuffer& a_Other);

private:

    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
    UINT m_NumIndices;
};

