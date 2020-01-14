
#include <iostream>
#include "d3d12.h"
#include "wrl.h"

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "comdef.h"
#include <dxgi1_6.h>

#include "d3dx12.h"

#include "io.h"
#include <fcntl.h>
#include "DirectXMath.h"
#include "d3dcompiler.h"
#include <filesystem>
#include "Helpers.h"

using namespace Microsoft::WRL;

const int g_ScreenWidth = 800;
const int g_ScreenHeight = 800;
const int g_BufferCount = 3;
int g_CurrentBuffer = 0;

ComPtr<ID3D12Device2> g_D3DDevice;
ComPtr<IDXGISwapChain> g_SwapChain;

uint64_t g_FenceValue = 0;
ComPtr<ID3D12Fence1> g_Fence;
ComPtr<ID3D12CommandQueue> g_CommandQueue;
ComPtr<ID3D12CommandAllocator> g_CommandAllocator;
ComPtr<ID3D12GraphicsCommandList> g_CommandList;

UINT g_rtvDescriptorSize;
UINT g_dsvDescriptorSize;
UINT g_cbvDescriptorSize;


ComPtr<ID3D12DescriptorHeap> g_RtvDescriptorHeap;
ComPtr<ID3D12DescriptorHeap> g_DsvDescriptorHeap;

ComPtr<ID3D12Resource> g_BackBufferResources[g_BufferCount];
ComPtr<ID3D12Resource> g_DepthStencilBuffer;

D3D12_VIEWPORT g_Viewport = {};
D3D12_RECT g_ScissorRect = {};

ComPtr<ID3D12Resource> g_VertexBufferResource;
ComPtr<ID3D12Resource> g_IntermediateResource;
D3D12_VERTEX_BUFFER_VIEW g_VertexBufferView;

ComPtr<ID3D12RootSignature> g_RootSignature;
ComPtr<ID3D12PipelineState> g_PSO;

DirectX::XMMATRIX MVP;

D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(g_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), g_CurrentBuffer, g_rtvDescriptorSize);
};

D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()
{
    return g_DsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
};
//
//inline void ThrowIfFailed(HRESULT hResult)
//{
//    if (FAILED(hResult))
//    {
//        _com_error err(hResult);
//        throw std::exception();
//    }
//}

void RedirectIOToConsole()
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

void FlushCommandQueue();

void Render();

LRESULT CALLBACK WindowsCallbackk(HWND a_HWND, UINT a_Message, WPARAM a_WParam, LPARAM a_LParam)
{
    switch (a_Message)
    {
    case WM_PAINT:
        Render();
    default:
        return ::DefWindowProcW(a_HWND, a_Message, a_WParam, a_LParam);
    }

};



struct WindowInfo
{
    HINSTANCE m_HInstance = NULL;
    LRESULT(*m_WndProc)(HWND, UINT, WPARAM, LPARAM) = nullptr;
    LPCWSTR m_ClassName = L"TangraEngine";

    const wchar_t* m_WindowTitle = L"TangraEngine";
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
};

HWND CreateApplicationWindow(WindowInfo& a_WindowInfo)
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

int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR lpCmdLine, _In_ int nCmdShow)
{
    hPrevInstance;
    lpCmdLine;
    nCmdShow;
    RedirectIOToConsole();
    using namespace Microsoft::WRL;
    ComPtr<IDXGIFactory4> dxgiFactory;

    UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
    dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    std::cout << "Creating window..." << std::endl;

    WindowInfo wInfo;
    wInfo.m_HInstance = hInstance;
    wInfo.m_ClassName = L"TangraEngine";
    wInfo.m_WindowTitle = L"Tangra";
    wInfo.m_Width = g_ScreenWidth;
    wInfo.m_Height = g_ScreenHeight;
    wInfo.m_WndProc = &WindowsCallbackk;

    HWND windowHandle = CreateApplicationWindow(wInfo);

    std::cout << "Creating DXGI factory..." << std::endl;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

    std::cout << "Querying DXGI Adapters..." << std::endl;
    ComPtr<IDXGIAdapter4> dxgiAdapterToUse = nullptr;
    size_t adapterVideoMemory = 0;

    {
        UINT i = 0;
        ComPtr<IDXGIAdapter1> adapter = nullptr;
        while (dxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC1 adapterDesc;

            adapter->GetDesc1(&adapterDesc);

            bool isSoftware = (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
            if (!isSoftware && adapterDesc.DedicatedVideoMemory > adapterVideoMemory &&
                SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)))
            {
                adapterVideoMemory = adapterDesc.DedicatedVideoMemory;
                ThrowIfFailed(adapter.As(&dxgiAdapterToUse));
            }
            ++i;
        }
    }

    if (dxgiAdapterToUse.Get() == nullptr)
    {
        std::cout << "Could not find suitable Graphics Adapter, stopping process..." << std::endl;
        abort();
    }

    DXGI_ADAPTER_DESC1 desc;

    dxgiAdapterToUse->GetDesc1(&desc);

    std::wcout << desc.Description << std::endl;

#ifdef _DEBUG
    std::cout << "Application running in debug mode, enabling debug layer..." << std::endl;

    ComPtr<ID3D12Debug> d3d12DebugInterface;

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12DebugInterface))))
    {
        d3d12DebugInterface->EnableDebugLayer();
    }

#endif


    std::cout << "Creating D3D12 Device..." << std::endl;
    ThrowIfFailed(D3D12CreateDevice(dxgiAdapterToUse.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&g_D3DDevice)));

    g_rtvDescriptorSize = g_D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    g_dsvDescriptorSize = g_D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    g_cbvDescriptorSize = g_D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ThrowIfFailed(g_D3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_Fence)));

    std::cout << "Creating Command Queue, Allocator and List..." << std::endl;

    {
        D3D12_COMMAND_QUEUE_DESC directCommandQueueDesc;
        directCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        directCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        directCommandQueueDesc.NodeMask = 0;
        directCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

        ThrowIfFailed(g_D3DDevice->CreateCommandQueue(&directCommandQueueDesc, IID_PPV_ARGS(&g_CommandQueue)));
        ThrowIfFailed(g_D3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_CommandAllocator)));
        ThrowIfFailed(g_D3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&g_CommandList)));
    }
    std::cout << "Creating DXGI Swap Chain..." << std::endl;
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    {
        swapChainDesc.BufferDesc.Width = g_ScreenWidth;
        swapChainDesc.BufferDesc.Height = g_ScreenHeight;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferDesc.Format = backBufferFormat;
        swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;

        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        swapChainDesc.Windowed = true;

        swapChainDesc.BufferCount = g_BufferCount;
        swapChainDesc.OutputWindow = windowHandle;

        ThrowIfFailed(dxgiFactory->CreateSwapChain(g_CommandQueue.Get(), &swapChainDesc, g_SwapChain.GetAddressOf()));
    }

    std::cout << "Creating RTV and DSV descriptor heaps..." << std::endl;

    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
        rtvDescriptorHeapDesc.NumDescriptors = g_BufferCount;
        rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtvDescriptorHeapDesc.NodeMask = 0;

        ThrowIfFailed(g_D3DDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&g_RtvDescriptorHeap)));
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
        dsvDescriptorHeapDesc.NumDescriptors = 1;
        dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvDescriptorHeapDesc.NodeMask = 0;

        ThrowIfFailed(g_D3DDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&g_DsvDescriptorHeap)));
    }

    std::cout << "Creating RTVs for the swap chain..." << std::endl;
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(g_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < g_BufferCount; i++)
    {
        ThrowIfFailed(g_SwapChain->GetBuffer(i, IID_PPV_ARGS(&g_BackBufferResources[i])));

        g_D3DDevice->CreateRenderTargetView(g_BackBufferResources[i].Get(), nullptr, rtvHeapHandle);

        // move the heap handle to the next entry in the heap
        rtvHeapHandle.Offset(1, g_rtvDescriptorSize);
    }

    std::cout << "Creating Depth/Stencil Buffer and View..." << std::endl;
    {
        D3D12_RESOURCE_DESC dsvResourceDesc = {};
        dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        dsvResourceDesc.Alignment = 0;
        dsvResourceDesc.Width = g_ScreenWidth;
        dsvResourceDesc.Height = g_ScreenHeight;
        dsvResourceDesc.DepthOrArraySize = 1;
        dsvResourceDesc.MipLevels = 1;
        dsvResourceDesc.SampleDesc.Count = 1;
        dsvResourceDesc.SampleDesc.Quality = 0;
        dsvResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        ThrowIfFailed(g_D3DDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
            &dsvResourceDesc, D3D12_RESOURCE_STATE_COMMON,
            nullptr, IID_PPV_ARGS(&g_DepthStencilBuffer)));
    }

    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(g_DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        g_D3DDevice->CreateDepthStencilView(g_DepthStencilBuffer.Get(), nullptr, DepthStencilView());
        g_CommandList->ResourceBarrier(1, &barrier);
    }

    // define viewport and scissor rect
    g_Viewport.TopLeftX = 0.0f;
    g_Viewport.TopLeftY = 0.0f;
    g_Viewport.Width = static_cast<float>(g_ScreenWidth);
    g_Viewport.Height = static_cast<float>(g_ScreenHeight);
    g_Viewport.MinDepth = 0.0f;
    g_Viewport.MaxDepth = 1.0f;

    g_ScissorRect.left = 0;
    g_ScissorRect.top = 0;
    g_ScissorRect.bottom = g_ScreenHeight;
    g_ScissorRect.right = g_ScreenWidth;

    // create vertex buffer for the triangle
    DirectX::XMFLOAT3 buffer[] = { DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f),DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f) };

    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(3 * sizeof(DirectX::XMFLOAT3));

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    g_D3DDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&g_VertexBufferResource));
    heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    g_D3DDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&g_IntermediateResource));

    D3D12_SUBRESOURCE_DATA subresourceData;
    subresourceData.pData = &buffer[0];
    subresourceData.RowPitch = 3 * sizeof(DirectX::XMFLOAT3);
    subresourceData.SlicePitch = subresourceData.RowPitch;

    UpdateSubresources(g_CommandList.Get(), g_VertexBufferResource.Get(), g_IntermediateResource.Get(),
        0, 0, 1, &subresourceData);


    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(g_VertexBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    g_CommandList->ResourceBarrier(1, &barrier);

    g_VertexBufferView.BufferLocation = g_VertexBufferResource->GetGPUVirtualAddress();
    g_VertexBufferView.SizeInBytes = sizeof(buffer);
    g_VertexBufferView.StrideInBytes = sizeof(DirectX::XMFLOAT3);

    namespace fs = std::experimental::filesystem;

    fs::path workPath = fs::current_path();

    std::cout << workPath << std::endl;

    fs::directory_iterator dirIter(workPath);
    bool assetsFound = false;
    while (!assetsFound)
    {
        for (auto& iter : dirIter)
        {
            if (iter.path().filename() == "Assets")
            {
                std::cout << iter.path() << std::endl;
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
                std::cout << "ERROR: Could not find Assets folder, unable to create rendering pipeline." << std::endl;
            }
        }
    }


    //create PSO and root signature

    using namespace Microsoft::WRL;
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;

    rootSignatureDesc.NumParameters = 1;

    CD3DX12_ROOT_PARAMETER rootParameters[1];
    rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    rootSignatureDesc.Init(1, rootParameters, 0, nullptr, rootSignatureFlags);

    ComPtr<ID3DBlob> rootSignatureSerialized;
    ComPtr<ID3DBlob> errorBlob;

    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSignatureSerialized, &errorBlob));

    ThrowIfFailed(g_D3DDevice->CreateRootSignature(0, rootSignatureSerialized->GetBufferPointer(), rootSignatureSerialized->GetBufferSize(), IID_PPV_ARGS(&g_RootSignature)));

    D3D12_INPUT_ELEMENT_DESC inputElements[1] = {};
    inputElements[0] = {};
    inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElements[0].AlignedByteOffset = 0;
    inputElements[0].InputSlot = 0;
    inputElements[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElements[0].SemanticName = "POSITION";
    inputElements[0].SemanticIndex = 0;
    inputElements[0].InstanceDataStepRate = 0;

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
    inputLayoutDesc.NumElements = 1;
    inputLayoutDesc.pInputElementDescs = inputElements;

    ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;

    std::cout << std::experimental::filesystem::current_path();


    ThrowIfFailed(D3DReadFileToBlob(L"ShaderCSOs/VertexShader.cso", &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"ShaderCSOs/PixelShader.cso", &pixelShaderBlob));

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL depthStencil;
    } pipelineStateStream;

    pipelineStateStream.pRootSignature = g_RootSignature.Get();

    CD3DX12_DEFAULT def;

    CD3DX12_DEPTH_STENCIL_DESC depthDesc = CD3DX12_DEPTH_STENCIL_DESC(def);
    //depthDesc.DepthEnable = false;
    pipelineStateStream.depthStencil = depthDesc;

    pipelineStateStream.InputLayout = inputLayoutDesc;
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = backBufferFormat;

    MVP = DirectX::XMMatrixOrthographicLH(g_ScreenWidth, g_ScreenHeight, 0.00001f, 100000.0f);

    MVP = DirectX::XMMatrixMultiply(MVP, DirectX::XMMatrixScaling(100.0f, 100.0f, 100.0f));

    pipelineStateStream.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc;
    pipelineStateStreamDesc.SizeInBytes = sizeof(pipelineStateStream);
    pipelineStateStreamDesc.pPipelineStateSubobjectStream = &pipelineStateStream;

    ThrowIfFailed(g_D3DDevice->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&g_PSO)));

    // close and execute command list before starting to render
    ThrowIfFailed(g_CommandList->Close());

    ID3D12CommandList* commandListPtr[] = { g_CommandList.Get() };

    g_CommandQueue->ExecuteCommandLists(1, commandListPtr);

    FlushCommandQueue();

    ::ShowWindow(windowHandle, SW_SHOW);
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    return 0;
}

void FlushCommandQueue()
{
    g_FenceValue++;
    ThrowIfFailed(g_CommandQueue->Signal(g_Fence.Get(), g_FenceValue));

    if (g_Fence->GetCompletedValue() < g_FenceValue)
    {
        HANDLE eventHandle = CreateEventExW(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(g_Fence->SetEventOnCompletion(g_FenceValue, eventHandle));

        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void Render()
{
    ThrowIfFailed(g_CommandAllocator->Reset());
    ThrowIfFailed(g_CommandList->Reset(g_CommandAllocator.Get(), nullptr));

    D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle = CurrentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle = DepthStencilView();

    auto currentRTV = g_BackBufferResources[g_CurrentBuffer];

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(currentRTV.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    g_CommandList->ResourceBarrier(1, &barrier);



    FLOAT clearColor[4] = { 0.4f, 0.5f, 0.9f, 1.0f };
    g_CommandList->ClearRenderTargetView(CurrentBackBufferView(), clearColor, 0, nullptr);
    g_CommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    g_CommandList->SetPipelineState(g_PSO.Get());
    g_CommandList->SetGraphicsRootSignature(g_RootSignature.Get());

    g_CommandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    g_CommandList->RSSetViewports(1, &g_Viewport);
    g_CommandList->RSSetScissorRects(1, &g_ScissorRect);

    g_CommandList->OMSetRenderTargets(1, &backBufferHandle, true, &depthStencilHandle);
    g_CommandList->IASetVertexBuffers(0, 1, &g_VertexBufferView);

    g_CommandList->SetGraphicsRoot32BitConstants(0, sizeof(MVP) / 4, &MVP, 0);

    g_CommandList->DrawInstanced(3, 1, 0, 0);


    barrier = CD3DX12_RESOURCE_BARRIER::Transition(currentRTV.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    g_CommandList->ResourceBarrier(1, &barrier);

    ThrowIfFailed(g_CommandList->Close());

    ID3D12CommandList* commandLists[] = { g_CommandList.Get() };

    g_CommandQueue->ExecuteCommandLists(1, commandLists);

    ThrowIfFailed(g_SwapChain->Present(0, 0));

    g_CurrentBuffer = (g_CurrentBuffer + 1) % g_BufferCount;

    FlushCommandQueue();
}