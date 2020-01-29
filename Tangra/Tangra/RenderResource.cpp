#include "RenderResource.h"

#include "DirectXTex.h"

void RenderResource::SetName(std::wstring a_NewName)
{
    m_DefaultBuffer->SetName(a_NewName.c_str());
    m_UploadBuffer->SetName((a_NewName + L" Upload buffer").c_str());
}

RenderResource::~RenderResource()
{
    
}
