#pragma once
#include <memory>

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "wrl.h"
#include "d3d12.h"
#include "dxgi1_6.h"
#include "Texture.h"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif


class Device;
class CommandQueue;
class SwapChain;
class VertexBuffer;
class PipelineState;
class IndexBuffer;

LRESULT CALLBACK WindowsCallback(HWND a_HWND, UINT a_Message, WPARAM a_WParam, LPARAM a_LParam);

// The Application class is a singleton to ensure that the WndProc function can access the instance to process Windows events
class Application
{
public:

    struct InitInfo
    {
        HINSTANCE m_HInstance = NULL;

        const wchar_t* m_WindowTitle = L"TangraEngine";
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        uint8_t m_NumBuffers = 2;

        bool m_CreateDebugConsole = true;
    };

    static void Create(InitInfo& a_InitInfo);
    static Application* Get();
    static void Destroy();

    void Run();

    LRESULT ProcessCallback(HWND a_HWND, UINT a_Message, WPARAM a_WParam, LPARAM a_LParam);

    void Initialize(InitInfo& a_InitInfo);

    Device* GetDevice();
    Microsoft::WRL::ComPtr<IDXGIFactory1> GetDXGIFactory();

    uint32_t GetScreenWidth() const;
    uint32_t GetScreenHeight() const;

private:

    struct WindowInfo
    {
        HINSTANCE m_HInstance = NULL;
        LRESULT(*m_WndProc)(HWND, UINT, WPARAM, LPARAM) = nullptr;
        LPCWSTR m_ClassName = L"TangraEngine";

        const wchar_t* m_WindowTitle = L"TangraEngine";
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
    };


    Application();

    void CreateDebugConsole();
    HWND CreateWindowInstance(WindowInfo& a_WindowInfo);
    void CreateDXGIFactory();
    Microsoft::WRL::ComPtr<IDXGIAdapter4> QueryGraphicsAdapters();
    void CreateD3D12Device(Microsoft::WRL::ComPtr<IDXGIAdapter4> a_graphicsAdapter);
    void LoadPSOs();

    void Render();

    static Application* ms_Instance;
    static bool ms_Initialized;

    // renderer variables
    uint32_t m_ScreenWidth;
    uint32_t m_ScreenHeight;

    // d3d12 objects
    Microsoft::WRL::ComPtr<IDXGIFactory1> m_DXGIFactory;
    std::unique_ptr<Device> m_D3DDevice;
    std::unique_ptr<SwapChain> m_SwapChain;
    std::unique_ptr<CommandQueue> m_DirectCommandQueue;

    std::unique_ptr<VertexBuffer> m_Buffer;
    std::unique_ptr<IndexBuffer> m_IndexBuffer;
    std::unique_ptr<Texture> m_Texture;

    std::unique_ptr<PipelineState> m_MainPSO;

    RECT m_ScissorRect;
    D3D12_VIEWPORT m_Viewport;

    HWND m_HWND;
};


