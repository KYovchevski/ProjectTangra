#pragma once
#include <memory>

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "wrl.h"
#include "d3d12.h"
#include "dxgi1_6.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

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
struct Mesh;

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

    // Application creation is done via a static function due to the dependency of DirectX 12 on WndProc
    static void Create(InitInfo& a_InitInfo);
    static void Destroy();

    static void Run();

    // Called in the WindowsCallback to process the events
    LRESULT ProcessCallback(HWND a_HWND, UINT a_Message, WPARAM a_WParam, LPARAM a_LParam);

    // Initialization code for DirectX 12 and the window
    void Initialize(InitInfo& a_InitInfo);

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

    // Create a system console to use for debug output
    void CreateDebugConsole();
    HWND CreateWindowInstance(WindowInfo& a_WindowInfo);
    void CreateDXGIFactory();
    // Gets the graphics adapter with the most dedicated VRAM
    Microsoft::WRL::ComPtr<IDXGIAdapter4> QueryGraphicsAdapters();

    // Function to hide the code used for creation of the PSOs since currently it's very bulky
    void LoadPSOs();

    void Render();

    // renderer variables
    uint32_t m_ScreenWidth;
    uint32_t m_ScreenHeight;

    // d3d12 objects
    Microsoft::WRL::ComPtr<IDXGIFactory1> m_DXGIFactory;

    VertexBuffer m_Buffer;
    IndexBuffer m_IndexBuffer;
    Texture m_Texture;

    Mesh* m_FoxMesh;

    std::unique_ptr<PipelineState> m_MainPSO;

    RECT m_ScissorRect;
    D3D12_VIEWPORT m_Viewport;

    HWND m_HWND;
};


