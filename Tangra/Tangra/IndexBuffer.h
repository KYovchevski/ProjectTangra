#pragma once
#include "RenderResource.h"

#include "d3dx12.h"

#include <vector>
#include "iostream"

class GraphicsCommandList;
///@brief 
///Wrapper class around a D3D12 resource that is used as an index buffer.
///Use a command list instance to create an index buffer.
///
class IndexBuffer :
    public RenderResource
{
public:

    IndexBuffer();
    //Used by command list class
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

