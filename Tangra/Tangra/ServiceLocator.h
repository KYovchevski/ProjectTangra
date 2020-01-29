#pragma once
#include <memory>

class Application;
class Device;
class SwapChain;

/*
 * Service Locator struct to avoid using a singleton.
 * It appears to be impossible to skip using this or a singleton pattern with DirectX 12 due to the dependency on WndProc
 */
struct ServiceLocator
{
    std::unique_ptr<Application> m_App;
    std::unique_ptr<Device>      m_Device;
    std::unique_ptr<SwapChain>   m_SwapChain;
};
