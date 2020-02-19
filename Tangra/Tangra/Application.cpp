#include "Application.h"

#include "GraphicsCommandList.h"
#include "CommandQueue.h"
#include "Device.h"
#include "SwapChain.h"
#include "PipelineState.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ModelLoader.h"

#include "ServiceLocator.h"

#include "Helpers.h"

#include <filesystem>
#include <assert.h>
#include <iostream>
#include <fcntl.h>
#include <io.h>
#include <algorithm>
#include <d3dcompiler.h>

namespace fs = std::experimental::filesystem;
//
//Application* Application::ms_Instance = nullptr;
//bool Application::ms_Initialized = false;

// Global ServiceLocator which holds the Application instance. Needed to provide access to the application in WndProc
ServiceLocator g_ServiceLocator;

using namespace Microsoft::WRL;

void Application::Create(InitInfo& a_InitInfo)
{
    if (g_ServiceLocator.m_App == nullptr)
    {
        g_ServiceLocator.m_App = std::unique_ptr<Application>(new Application());

        g_ServiceLocator.m_App->Initialize(a_InitInfo);
    }
    else
    {
        std::cout << "ERROR: Application instance is already created, cannot create another one." << std::endl;
    }
}

void Application::Destroy()
{
    if (g_ServiceLocator.m_App)
    {
        g_ServiceLocator.m_App.reset();
        std::cout << "Application instance destroyed." << std::endl;
    }
}

void Application::Run()
{
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
}

LRESULT Application::ProcessCallback(HWND a_HWND, UINT a_Message, WPARAM a_WParam, LPARAM a_LParam)
{
    switch (a_Message)
    {
    case WM_PAINT:
        Render();
        break;
    default:
        return ::DefWindowProcW(a_HWND, a_Message, a_WParam, a_LParam);
    }
    return 0;
}

void Application::Initialize(InitInfo& a_InitInfo)
{
    ThrowIfFailed(CoInitialize(NULL));

    // if requested allocate a console to output debug information to
    if (a_InitInfo.m_CreateDebugConsole)
    {
        CreateDebugConsole();
    }

    WindowInfo wInfo = {};
    wInfo.m_ClassName = L"TangraRenderer";
    wInfo.m_WndProc = &WindowsCallback;
    //wInfo.m_HInstance = a_InitInfo.m_HInstance;
    wInfo.m_HInstance = (HINSTANCE)GetModuleHandle(NULL);
    wInfo.m_Height = a_InitInfo.m_Height;
    wInfo.m_Width = a_InitInfo.m_Width;
    wInfo.m_WindowTitle = a_InitInfo.m_WindowTitle;

    m_HWND = CreateWindowInstance(wInfo);

    m_ScreenWidth = a_InitInfo.m_Width;
    m_ScreenHeight = a_InitInfo.m_Height;

    CreateDXGIFactory();
    auto graphicsAdapter = QueryGraphicsAdapters();

    // dirty way to find the assets folder regardless if the app is ran via the .exe or via Visual studio

    fs::path workPath = fs::current_path();
    fs::directory_iterator dirIter(workPath);
    bool assetsFound = false;
    while (!assetsFound)
    {
        for (auto & iter : dirIter)
        {
            if (iter.path().filename() == "Assets")
            {
                fs::current_path(iter.path());
                assetsFound = true;
                break;
            }
        }
        if (!assetsFound)
        {
            if (workPath.has_parent_path())
            {
                workPath = workPath.parent_path();
                dirIter = fs::directory_iterator(workPath);
            }
            else
            {
                std::cout << "ERROR: Could not find Assets folder, unable to locate shaders." << std::endl;
            }
        }
    }


    std::cout << "Working directory set to: " << fs::current_path() << std::endl;
    std::cout << "Creating direct command queue" << std::endl;

    g_ServiceLocator.m_Device = std::make_unique<Device>(graphicsAdapter, g_ServiceLocator);
    g_ServiceLocator.m_Device->Initialize();
    CommandQueue* commandQueue = g_ServiceLocator.m_Device->GetCommandQueue();

    g_ServiceLocator.m_SwapChain = std::make_unique<SwapChain>(g_ServiceLocator, m_HWND, a_InitInfo.m_NumBuffers);
    std::cout << "Creating descriptor heaps" << std::endl;
    g_ServiceLocator.m_SwapChain->CreateDescriptorHeaps();

    GraphicsCommandList* commandList = commandQueue->GetCommandList();

    std::cout << "Creating RTVs" << std::endl;
    g_ServiceLocator.m_SwapChain->CreateRenderTargets();

    
    std::cout << "Creating depth stencil buffer" << std::endl;
    g_ServiceLocator.m_SwapChain->CreateDepthStencilBuffer(*commandList);

    std::cout << "Creating triangle vertex buffer" << std::endl;

    struct vertex
    {
        DirectX::XMFLOAT3 p;
        DirectX::XMFLOAT2 t;
    };

    vertex v1, v2, v3;
    v1 = vertex{ DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f), DirectX::XMFLOAT2(0.0f, 0.0f) };
    v2 = vertex{ DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), DirectX::XMFLOAT2(1.0f, 1.0f) };
    v3 = vertex{ DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f) , DirectX::XMFLOAT2(1.0f, 0.0f) };

    std::vector<vertex> vertices = { v1, v2, v3 };
    std::vector<UINT> indices = { 0, 1, 2 };
    //std::reverse(vertices.begin(), vertices.end());
    //m_Buffer = std::make_unique<VertexBuffer>(g_ServiceLocator, vertices, *commandList);
    m_Buffer = commandList->CreateVertexBuffer(vertices);

    //m_IndexBuffer = std::make_unique<IndexBuffer>(g_ServiceLocator, indices, *commandList);
    m_IndexBuffer = commandList->CreateIndexBuffer(indices);

    std::wstring path = L"Models/Fox/glTF/Texture.png";

    m_Texture = commandList->CreateTextureFromFilePath(path);

    std::cout << "Creating pipeline state object" << std::endl;
    LoadPSOs();


    m_Viewport.TopLeftX = 0.0f;
    m_Viewport.TopLeftY = 0.0f;
    m_Viewport.Width = static_cast<float>(m_ScreenWidth);
    m_Viewport.Height = static_cast<float>(m_ScreenHeight);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;
    
    m_ScissorRect.left = 0;
    m_ScissorRect.top = 0;
    m_ScissorRect.bottom = m_ScreenHeight;
    m_ScissorRect.right = m_ScreenWidth;



    commandQueue->ExecuteCommandList(*commandList);
    commandQueue->Flush();

    g_ServiceLocator.m_ModelLoader = std::make_unique<ModelLoader>(g_ServiceLocator);

    m_FoxMesh = &g_ServiceLocator.m_ModelLoader->LoadMeshFromGLTF("Models/Fox/glTF/Fox.gltf");
    //m_FoxMesh = &g_ServiceLocator.m_ModelLoader->LoadMeshFromGLTF("Models/RiggedSimple/glTF/RiggedSimple.gltf");
    //m_FoxMesh = &g_ServiceLocator.m_ModelLoader->LoadMeshFromGLTF("Models/Fox/glTF/Fox.gltf");
    //m_FoxMesh = &g_ServiceLocator.m_ModelLoader->LoadMeshFromGLTF("Models/Fox/glTF/Fox.gltf");
    //m_FoxMesh = &g_ServiceLocator.m_ModelLoader->LoadMeshFromGLTF("Models/Fox/glTF/Fox.gltf");

    std::cout << "Initialization completed." << std::endl;
    ::ShowWindow(m_HWND, SW_SHOW);

}

Microsoft::WRL::ComPtr<IDXGIFactory1> Application::GetDXGIFactory()
{
    return m_DXGIFactory;
}

uint32_t Application::GetScreenWidth() const
{
    return m_ScreenWidth;
}

uint32_t Application::GetScreenHeight() const
{
    return m_ScreenHeight;
}

HWND Application::CreateWindowInstance(Application::WindowInfo& a_WindowInfo)
{
    if (a_WindowInfo.m_Width == 0 || a_WindowInfo.m_Height == 0)
    {
        throw std::exception("Cannot create window with width or height of 0.");
    }

    WNDCLASSEXW windowClass = {};

    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    // CALLBACK function that Windows uses to process messages
    windowClass.lpfnWndProc = a_WindowInfo.m_WndProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    // HINSTANCE, provided by WinMain function
    windowClass.hInstance = a_WindowInfo.m_HInstance;
    windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    windowClass.hIcon = ::LoadIcon(a_WindowInfo.m_HInstance, IDI_APPLICATION);
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    windowClass.lpszMenuName = NULL;
    // name of the window class, needs to be unique
    windowClass.lpszClassName = a_WindowInfo.m_ClassName;

    // final class registration
    static ATOM atom = ::RegisterClassExW(&windowClass);
    assert(atom > 0);

    // get system's screen width and height in pixels
    int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect = { 0, 0, static_cast<LONG>(a_WindowInfo.m_Width), static_cast<LONG>(a_WindowInfo.m_Height) };
    // adjusts the size of the window to account for the borders of the window
    ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    // position the window in the centre of the screen
    int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
    int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

    HWND hwnd = CreateWindowExW(
        NULL,
        a_WindowInfo.m_ClassName,
        a_WindowInfo.m_WindowTitle,
        WS_OVERLAPPEDWINDOW,
        windowX, windowY,
        windowWidth, windowHeight,
        NULL, NULL, a_WindowInfo.m_HInstance, nullptr);

    assert(hwnd && "Window creation failed.");
    return hwnd;
}

Application::Application()
    : m_ScissorRect(RECT())
    , m_Viewport(D3D12_VIEWPORT())
{
    m_ScissorRect = RECT();
    m_ScreenHeight = 0;
    m_ScreenWidth = 0;
    m_HWND = NULL;
}

void Application::CreateDebugConsole()
{
    AllocConsole();

    HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    int SystemOutput = _open_osfhandle(intptr_t(ConsoleOutput), _O_TEXT);
    FILE* COutputHandle = _fdopen(SystemOutput, "w");

    // Get STDERR handle
    HANDLE ConsoleError = GetStdHandle(STD_ERROR_HANDLE);
    int SystemError = _open_osfhandle(intptr_t(ConsoleError), _O_TEXT);
    FILE* CErrorHandle = _fdopen(SystemError, "w");

    // Get STDIN handle
    HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
    int SystemInput = _open_osfhandle(intptr_t(ConsoleInput), _O_TEXT);
    FILE* CInputHandle = _fdopen(SystemInput, "r");

    //make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
    std::ios::sync_with_stdio(true);

    // Redirect the CRT standard input, output, and error handles to the console
    freopen_s(&CInputHandle, "CONIN$", "r", stdin);
    freopen_s(&COutputHandle, "CONOUT$", "w", stdout);
    freopen_s(&CErrorHandle, "CONOUT$", "w", stderr);

    //Clear the error state for each of the C++ standard stream objects. We need to do this, as
    //attempts to access the standard streams before they refer to a valid target will cause the
    //iostream objects to enter an error state. In versions of Visual Studio after 2005, this seems
    //to always occur during startup regardless of whether anything has been read from or written to
    //the console or not.
    std::wcout.clear();
    std::cout.clear();
    std::wcerr.clear();
    std::cerr.clear();
    std::wcin.clear();
    std::cin.clear();
}

void Application::CreateDXGIFactory()
{
    std::cout << "Creating DXGI factory" << std::endl;

    UINT factoryCreationFlags = 0;
#ifdef _DEBUG
    factoryCreationFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(factoryCreationFlags, IID_PPV_ARGS(&m_DXGIFactory)));
}

ComPtr<IDXGIAdapter4> Application::QueryGraphicsAdapters()
{
    std::cout << "Querying DXGI Adapters..." << std::endl;
    ComPtr<IDXGIAdapter4> dxgiAdapterToUse = nullptr;
    size_t adapterVideoMemory = 0;

    {
        UINT i = 0;
        ComPtr<IDXGIAdapter1> adapter = nullptr;
        // enumerate the adapters until all have been enumerated, resulting in EnumAdapters returning DXGI_ERROR_NOT_FOUND
        while (m_DXGIFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC1 adapterDesc;

            adapter->GetDesc1(&adapterDesc);

            bool isSoftware = (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;

            // check if the adapter is a software adapter and if it has more video memory than the currently selected one
            if (!isSoftware && adapterDesc.DedicatedVideoMemory > adapterVideoMemory &&
                // create a fake D3D12Device on the adapter to ensure one can be created with the specified D3D_FEATURE_LEVEL
                SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)))
            {
                adapterVideoMemory = adapterDesc.DedicatedVideoMemory;
                ThrowIfFailed(adapter.As(&dxgiAdapterToUse));
            }
            ++i;
        }
    }


    DXGI_ADAPTER_DESC1 adapterDesc;

    dxgiAdapterToUse->GetDesc1(&adapterDesc);

    std::wcout << "Adapter to use: " << adapterDesc.Description << std::endl;

    return dxgiAdapterToUse;
}


void Application::LoadPSOs()
{
    PipelineState::InitializationData initData;
    initData.m_RootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1);

    initData.m_RootParameters.emplace_back();
    initData.m_RootParameters.back().InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    initData.m_RootParameters.emplace_back();
    initData.m_RootParameters.back().InitAsDescriptorTable(1u, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);
    initData.m_RootParameters.emplace_back();
    initData.m_RootParameters.back().InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    initData.m_StaticSamplers.emplace_back();
    initData.m_StaticSamplers.back().RegisterSpace = 1;
    initData.m_StaticSamplers.back().ShaderRegister = 0;
    initData.m_StaticSamplers.back().ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    initData.m_StaticSamplers.back().AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    initData.m_StaticSamplers.back().AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    initData.m_StaticSamplers.back().AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    initData.m_StaticSamplers.back().ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    initData.m_StaticSamplers.back().Filter = D3D12_FILTER_ANISOTROPIC;
    initData.m_StaticSamplers.back().MinLOD = 0;
    initData.m_StaticSamplers.back().MaxLOD = 0;


    D3D12_INPUT_ELEMENT_DESC* backElement = nullptr;
    initData.m_InputElements.emplace_back();
    backElement = &initData.m_InputElements.back();

    *backElement = {};
    backElement->Format = DXGI_FORMAT_R32G32B32_FLOAT;
    backElement->AlignedByteOffset = 0;
    backElement->InputSlot = 0;
    backElement->SemanticIndex = 0;
    backElement->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    backElement->SemanticName = "POSITION";
    backElement->InstanceDataStepRate = 0;

    initData.m_InputElements.emplace_back();
    backElement = &initData.m_InputElements.back();

    *backElement = {};
    backElement->Format = DXGI_FORMAT_R32G32_FLOAT;
    backElement->AlignedByteOffset = 0;
    backElement->InputSlot = 1;
    backElement->SemanticIndex = 0;
    backElement->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    backElement->SemanticName = "TEXCOORD";
    backElement->InstanceDataStepRate = 0;

    initData.m_InputElements.emplace_back();
    backElement = &initData.m_InputElements.back();

    *backElement = {};
    backElement->Format = DXGI_FORMAT_R32G32B32A32_UINT;
    backElement->AlignedByteOffset = 0;
    backElement->InputSlot = 2;
    backElement->SemanticIndex = 0;
    backElement->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    backElement->SemanticName = "JOINT_IDS";
    backElement->InstanceDataStepRate = 0;

    initData.m_InputElements.emplace_back();
    backElement = &initData.m_InputElements.back();

    *backElement = {};
    backElement->Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    backElement->AlignedByteOffset = 0;
    backElement->InputSlot = 3;
    backElement->SemanticIndex = 0;
    backElement->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    backElement->SemanticName = "WEIGHTS";
    backElement->InstanceDataStepRate = 0;


    initData.m_DSVFormat = g_ServiceLocator.m_SwapChain->GetDepthStencilFormat();
    initData.m_VertexShaderPath = L"ShaderCSOs/VertexShader.cso";
    initData.m_PixelShaderPath = L"ShaderCSOs/PixelShader.cso";
    initData.m_PrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    initData.m_RTVFormats.push_back(g_ServiceLocator.m_SwapChain->GetBackBufferFormat());

    m_MainPSO = std::make_unique<PipelineState>(initData, g_ServiceLocator);
}

void Application::Render()
{

    g_ServiceLocator.m_SwapChain->SetClearColor(DirectX::SimpleMath::Color(0.4f, 0.5f, 0.9f, 1.0f));


    auto directCommandQueue = g_ServiceLocator.m_Device->GetCommandQueue();
    // for simplicity, use a single command list for clearing, drawing and presenting
    auto commandList = directCommandQueue->GetCommandList();
    
    g_ServiceLocator.m_SwapChain->ClearBackBuffer(*commandList);
    g_ServiceLocator.m_SwapChain->ClearDSV(*commandList);

    auto srvHeap = g_ServiceLocator.m_Device->GetSRVHeap().Get();
       
    commandList->SetPipelineState(*m_MainPSO);
    commandList->SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetViewport(m_Viewport);
    commandList->SetScissorRect(m_ScissorRect);
    commandList->SetRenderTargets(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>{g_ServiceLocator.m_SwapChain->GetCurrentRTVHandle()}, TRUE, g_ServiceLocator.m_SwapChain->GetDSVHandle());
    commandList->SetVertexBuffer(m_FoxMesh->m_Positions, 0);
    commandList->SetVertexBuffer(m_FoxMesh->m_TexCoords, 1);
    commandList->SetVertexBuffer(m_FoxMesh->m_JointIDs, 2);
    commandList->SetVertexBuffer(m_FoxMesh->m_Weights, 3);
    //commandList->SetIndexBuffer(m_IndexBuffer);
    commandList->SetDescriptorHeap(srvHeap);
    commandList->SetTexture(1, m_Texture);


    namespace dx = DirectX;

    

    static float f = 0.0f, f1 = 130.0f;
    f += 3.0f / 1.0f;
    //f1 += 3.0f / 10.0f;

    //mat *= sm::Matrix::CreateRotationZ(DirectX::XMConvertToRadians(f));
    // const float sc = 3.0f;
    //mat *= sm::Matrix::CreateScale(sm::Vector3(sc, sc, sc));
    //
    namespace sm = DirectX::SimpleMath;

    sm::Matrix mat;
    mat = sm::Matrix::Identity;
    mat *= sm::Matrix::CreateRotationY(DirectX::XMConvertToRadians(f1));
    mat *= sm::Matrix::CreateTranslation(sm::Vector3(0.0f, 50.0f, 250.0f));
    mat *= sm::Matrix::CreateLookAt(Vector3(0.0f, 250.0f, -50.0f), Vector3(0.0f, 100.0f, 250.0f), Vector3(0.0f, 1.0f, 0.0f));
    //mat *= sm::Matrix::CreateOrthographic(float(m_ScreenWidth), float(m_ScreenHeight), 0.00001f, 100000.0f);
    mat *= sm::Matrix::CreatePerspectiveFieldOfView(DirectX::XMConvertToRadians(45.0f), float(m_ScreenWidth) / float(m_ScreenHeight), 0.01f, 10000.0f);// .Transpose();

    static float time = 0.0f;
    time += 1.0f / 60.0f;
    if (time > m_FoxMesh->m_Animation.m_Length)
    {
        time -= m_FoxMesh->m_Animation.m_Length;
    }

    m_FoxMesh->m_Skeleton.UpdateSkeleton(m_FoxMesh->m_Animation, time);
    std::vector<Matrix> skeletonMatrices = m_FoxMesh->m_Skeleton.GetMatrices();

    for (auto& joint : m_FoxMesh->m_Skeleton.m_Joints)
    {
        auto transl = joint->Transform.Translation();
        auto quat = joint->rotation;

        //std::printf("%s %f %f %f %f | %f %f %f %f \n", joint->name.c_str(), transl.x, transl.y, transl.z, transl.Length(), quat.x, quat.y, quat.z, quat.w);
    }

    auto& quat = m_FoxMesh->m_Skeleton.m_RootJoint->m_Children[0].m_Children[0].m_Children[2].rotation;
    quat = Quaternion::CreateFromYawPitchRoll(0.0f, 0.0f, DirectX::XMConvertToRadians(-f));

    commandList->SetRoot32BitConstant(0, mat);
    commandList->SetStructuredBuffer(2, skeletonMatrices);
    //commandList->GetCommandListPtr()->SetGraphicsRoot32BitConstants(0, sizeof(mat) / 4, &mat, 0);
    
    //commandList->DrawIndexed(m_IndexBuffer.GetNumIndices());
    commandList->Draw(m_FoxMesh->m_Positions.GetNumVertices());
    directCommandQueue->ExecuteCommandList(*commandList);

    g_ServiceLocator.m_SwapChain->Present();

}

LRESULT WindowsCallback(HWND a_HWND, UINT a_Message, WPARAM a_WParam, LPARAM a_LParam)
{
    return g_ServiceLocator.m_App->ProcessCallback(a_HWND, a_Message, a_WParam, a_LParam);
}
