#include "Application.h"

#include "GraphicsCommandList.h"
#include "CommandQueue.h"
#include "Device.h"
#include "SwapChain.h"
#include "PipelineState.h"

#include "Helpers.h"

#include <filesystem>
#include <assert.h>
#include <iostream>
#include <fcntl.h>
#include <io.h>
#include <algorithm>
#include <d3dcompiler.h>

namespace fs = std::experimental::filesystem;

Application* Application::ms_Instance = nullptr;
bool Application::ms_Initialized = false;

using namespace Microsoft::WRL;

void Application::Create(InitInfo& a_InitInfo)
{
    if (ms_Instance == nullptr)
    {
        ms_Instance = new Application();

        ms_Instance->Initialize(a_InitInfo);

        ms_Initialized = true;
    }
    else
    {
        std::cout << "ERROR: Application instance is already created, cannot create another one." << std::endl;
    }
}

Application* Application::Get()
{
    if (ms_Instance)
    {
        return ms_Instance;
    }
    else
    {
        std::cout << "ERROR: Create the Application before using it." << std::endl;
        return nullptr;
    }
}

void Application::Destroy()
{
    if (ms_Initialized)
    {
        delete ms_Instance;
        ms_Instance = nullptr;
        std::cout << "Application instance destroyed." << std::endl;

        ms_Initialized = false;
    }
}

void Application::Run()
{
    ::ShowWindow(m_HWND, SW_SHOW);
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
    default:
        return ::DefWindowProcW(a_HWND, a_Message, a_WParam, a_LParam);
    }
}

void Application::Initialize(InitInfo& a_InitInfo)
{
    // if requested allocate a console to output debug information to
    if (a_InitInfo.m_CreateDebugConsole)
    {
        CreateDebugConsole();
    }

    WindowInfo wInfo = {};
    wInfo.m_ClassName = L"TangraRenderer";
    wInfo.m_WndProc = &WindowsCallback;
    wInfo.m_HInstance = a_InitInfo.m_HInstance;
    wInfo.m_Height = a_InitInfo.m_Height;
    wInfo.m_Width = a_InitInfo.m_Width;
    wInfo.m_WindowTitle = a_InitInfo.m_WindowTitle;

    m_HWND = CreateWindowInstance(wInfo);

    m_ScreenWidth = a_InitInfo.m_Width;
    m_ScreenHeight = a_InitInfo.m_Height;

    CreateDXGIFactory();
    auto graphicsAdapter = QueryGraphicsAdapters();
    CreateD3D12Device(graphicsAdapter);

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

    m_DirectCommandQueue = std::make_unique<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_SwapChain = std::make_unique<SwapChain>(m_HWND, a_InitInfo.m_NumBuffers, *m_DirectCommandQueue);
    std::cout << "Creating descriptor heaps" << std::endl;
    m_SwapChain->CreateDescriptorHeaps();

    GraphicsCommandList* commandList = m_DirectCommandQueue->GetCommandList();

    std::cout << "Creating RTVs" << std::endl;
    m_SwapChain->CreateRenderTargets();

    
    std::cout << "Creating depth stencil buffer" << std::endl;
    m_SwapChain->CreateDepthStencilBuffer(*commandList);


    std::cout << "Creating triangle vertex buffer" << std::endl;
    std::vector<DirectX::XMFLOAT3> vertices = { DirectX::XMFLOAT3(1.f, 1.f, 0.2f),DirectX::XMFLOAT3(0.f, 0.0f, 0.2f), DirectX::XMFLOAT3(0.5f, 0.5f, 0.2f) };
    //std::reverse(vertices.begin(), vertices.end());
    m_Buffer = std::make_unique<VertexBuffer>(vertices, *commandList);

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

    m_DirectCommandQueue->ExecuteCommandList(*commandList);

    m_DirectCommandQueue->Flush();

    std::cout << "Initialization completed." << std::endl;
}

Device* Application::GetDevice()
{
    return m_D3DDevice.get();
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
{
    m_ScreenHeight = 0;
    m_ScreenWidth = 0;
    m_HWND = NULL;
    m_Buffer = std::make_unique<VertexBuffer>();
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

void Application::CreateD3D12Device(Microsoft::WRL::ComPtr<IDXGIAdapter4> a_graphicsAdapter)
{

    m_D3DDevice = std::make_unique<Device>(a_graphicsAdapter);
}

void Application::LoadPSOs()
{
    PipelineState::InitializationData initData;
    initData.m_RootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    initData.m_RootParameters.emplace_back();
    initData.m_RootParameters.back().InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

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


    initData.m_DSVFormat = m_SwapChain->GetDepthStencilFormat();
    initData.m_VertexShaderPath = L"ShaderCSOs/VertexShader.cso";
    initData.m_PixelShaderPath = L"ShaderCSOs/PixelShader.cso";
    initData.m_PrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    initData.m_RTVFormats.push_back(m_SwapChain->GetBackBufferFormat());

    m_MainPSO = std::make_unique<PipelineState>(initData);
}

void Application::Render()
{

    m_SwapChain->SetClearColor(DirectX::SimpleMath::Color(0.4f, 0.5f, 0.9f, 1.0f));

    // for simplicity, use a single command list for clearing, drawing and presenting
    auto commandList = m_DirectCommandQueue->GetCommandList();
    commandList->Reset();
    
    m_SwapChain->ClearBackBuffer(*commandList);
    m_SwapChain->ClearDSV(*commandList);
       
    commandList->SetPipelineState(*m_MainPSO);
    commandList->SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetViewport(m_Viewport);
    commandList->SetScissorRect(m_ScissorRect);
    commandList->SetRenderTargets(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>{m_SwapChain->GetCurrentRTVHandle()}, TRUE, m_SwapChain->GetDSVHandle());
    commandList->SetVertexBuffer(*m_Buffer);

    DirectX::XMMATRIX mat;

    mat = DirectX::XMMatrixOrthographicLH(float(m_ScreenWidth), float(m_ScreenHeight), 0.00001f, 100000.0f);
    
    mat = DirectX::XMMatrixMultiply(mat, DirectX::XMMatrixScaling(400.0f, 400.0f, 400.0f));
    

    commandList->SetRoot32BitConstant(0, mat);
    
    commandList->Draw(m_Buffer->GetNumVertices());

    m_SwapChain->Present(*commandList);

}

LRESULT WindowsCallback(HWND a_HWND, UINT a_Message, WPARAM a_WParam, LPARAM a_LParam)
{
    return Application::Get()->ProcessCallback(a_HWND, a_Message, a_WParam, a_LParam);
}
