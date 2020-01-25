#pragma once

#include "RenderResource.h"


#include <string>

class GraphicsCommandList;

class Texture
    : public RenderResource
{
public:
    Texture();

    Texture(std::wstring& a_FilePath, GraphicsCommandList& a_CommandList);

private:
    

};

