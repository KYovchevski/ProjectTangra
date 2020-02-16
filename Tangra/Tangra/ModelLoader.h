#pragma once

#include "gltf.h"
#include "Mesh.h"

struct ServiceLocator;

class ModelLoader
{
public:
    ModelLoader(ServiceLocator& a_Services) : m_Services(&a_Services){}

    Mesh& LoadMeshFromGLTF(std::string a_Filepath);


private:
    ServiceLocator* m_Services;

    std::map <std::string, Mesh> m_LoadedMeshes;
};

