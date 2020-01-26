
#include "Application.h"

#include "DirectXTex.h"
#include "filesystem"
#include <iostream>

using namespace DirectX;

namespace fs = std::experimental::filesystem;
int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR lpCmdLine, _In_ int nCmdShow)
{
    hPrevInstance;
    lpCmdLine;
    nCmdShow;

    Application::InitInfo appInitInfo;
    appInitInfo.m_NumBuffers = 3;
    appInitInfo.m_CreateDebugConsole = true;
    appInitInfo.m_HInstance = hInstance;
    appInitInfo.m_Height = 800;
    appInitInfo.m_Width = 800;
    appInitInfo.m_WindowTitle = L"Tongra cause retarded";
    Application::Create(appInitInfo);

    Application::Get()->Run();
}