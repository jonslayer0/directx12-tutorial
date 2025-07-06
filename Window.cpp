#include "Window.h"
#include "Application.h"

// Windows initiliazing function headers
void EnableDebugLayer();
bool CheckTearingSupport();
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName);
HWND CreateWindow(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle, uint32_t width, uint32_t height);

WINDOW::WINDOW(HINSTANCE hInstance, ComPtr<ID3D12CommandQueue> commandQueue)
{
    // Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
    // Using this awareness context allows the client area of the window 
    // to achieve 100% scaling while still allowing non-client window content to 
    // be rendered in a DPI sensitive fashion.
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Window class name. Used for registering / creating the window.
    const wchar_t* windowClassName = L"DX12WindowClass";
    ParseCommandLineArguments();

    EnableDebugLayer();

    _tearingSupported = CheckTearingSupport();

    RegisterWindowClass(hInstance, windowClassName);
    _hWnd = CreateWindow(windowClassName, hInstance, L"Learning DirectX 12", _clientWidth, _clientHeight);

    // Initialize the global window rect variable.
    ::GetWindowRect(_hWnd, &_windowRect);

    // Swap chain creation called by application
}

void WINDOW::ParseCommandLineArguments()
{
    int argc;
    wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

    for (size_t i = 0; i < argc; ++i)
    {
        if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0)
        {
            _clientWidth = ::wcstol(argv[++i], nullptr, 10);
        }

        if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0)
        {
            _clientHeight = ::wcstol(argv[++i], nullptr, 10);
        }

        if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
        {
            _useWarp = true;
        }
    }

    // Free memory allocated by CommandLineToArgvW
    ::LocalFree(argv);
}

void WINDOW::CreateSwapChain(ComPtr<ID3D12CommandQueue> commandQueue)
{
    ComPtr<IDXGISwapChain4> dxgiSwapChain4;
    ComPtr<IDXGIFactory4> dxgiFactory4;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = _clientWidth;
    swapChainDesc.Height = _clientHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = g_numFrames;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

    // It is recommended to always allow tearing if tearing support is available.
    swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
        commandQueue.Get(),
        _hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1));

    // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
    // will be handled manually.
    ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(_hWnd, DXGI_MWA_NO_ALT_ENTER));

    // Set swap chain
    ThrowIfFailed(swapChain1.As(&_swapChain));

    // Get first index of back buffer
    _currentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();
}

void WINDOW::Resize()
{
    RECT clientRect = {};
    ::GetClientRect(_hWnd, &clientRect);

    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    if (_clientWidth != width || _clientHeight != height)
    {
        // Don't allow 0 size swap chain back buffers.
        _clientWidth = std::max(1, width);
        _clientHeight = std::max(1, height);

        // Flush the GPU queue to make sure the swap chain's back buffers
        // are not being referenced by an in-flight command list.
        APPLICATION::Instance()->Flush();

        for (int i = 0; i < g_numFrames; ++i)
        {
            // Any references to the back buffers must be released
            // before the swap chain can be resized.
            _backBuffers[i].Reset();
            _frameFenceValues[i] = _frameFenceValues[_currentBackBufferIndex];
        }

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        ThrowIfFailed(_swapChain->GetDesc(&swapChainDesc));
        ThrowIfFailed(_swapChain->ResizeBuffers(g_numFrames, _clientWidth, _clientHeight,
            swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

        _currentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();

        UpdateRenderTargetViews(APPLICATION::Instance()->GetDevice(), APPLICATION::Instance()->GetDescriptorHeap());
    }
}

void WINDOW::SwitchFullscreen()
{
    _fullscreen = !_fullscreen;

    if (_fullscreen) // Switching to fullscreen.
    {
        // Store the current window dimensions so they can be restored 
        // when switching out of fullscreen state.
        ::GetWindowRect(_hWnd, &_windowRect);

        // Set the window style to a borderless window so the client area fills
        // the entire screen.
        UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
        ::SetWindowLongW(_hWnd, GWL_STYLE, windowStyle);

        // Query the name of the nearest display device for the window.
        // This is required to set the fullscreen dimensions of the window
        // when using a multi-monitor setup.
        HMONITOR hMonitor = ::MonitorFromWindow(_hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX monitorInfo = {};
        monitorInfo.cbSize = sizeof(MONITORINFOEX);
        ::GetMonitorInfo(hMonitor, &monitorInfo);

        ::SetWindowPos(_hWnd, HWND_TOP,
            monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.top,
            monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(_hWnd, SW_MAXIMIZE);
    }
    else
    {
        // Restore all the window decorators.
        ::SetWindowLong(_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

        ::SetWindowPos(_hWnd, HWND_NOTOPMOST,
            _windowRect.left,
            _windowRect.top,
            _windowRect.right - _windowRect.left,
            _windowRect.bottom - _windowRect.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(_hWnd, SW_NORMAL);
    }
}

void WINDOW::UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<ID3D12DescriptorHeap> descriptorHeap)
{
    auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < g_numFrames; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        ThrowIfFailed(_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

        _backBuffers[i] = backBuffer;

        rtvHandle.Offset(rtvDescriptorSize);
    }
}

void EnableDebugLayer()
{
#if defined(_DEBUG)
    // Always enable the debug layer before doing anything DX12 related
    // so all possible errors generated while creating DX12 objects
    // are caught by the debug layer.
    ComPtr<ID3D12Debug> debugInterface;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
#endif
}

bool CheckTearingSupport()
{
    BOOL allowTearing = false;

    // Rather than create the DXGI 1.5 factory interface directly, we create the
    // DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
    // graphics debugging tools which will not support the 1.5 factory interface
    // until a future update.

    ComPtr<IDXGIFactory4> factory4;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
    {
        ComPtr<IDXGIFactory5> factory5;
        if (SUCCEEDED(factory4.As(&factory5)))
        {
            if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
            {
                allowTearing = false;
            }
        }
    }

    return allowTearing == TRUE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    APPLICATION* application = APPLICATION::Instance();

    if (application && application->GetWindow()->isInitialized())
    {
        WINDOW* window = application->GetWindow();

        switch (message)
        {
        case WM_PAINT:
            application->Update();
            application->Render();
            break;
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

            switch (wParam)
            {
            case 'V':
                window->SwitchVSync();
                break;
            case VK_ESCAPE:
                ::PostQuitMessage(0);
                break;
            case VK_RETURN:
                if (alt)
                {
            case VK_F11:
                window->SwitchFullscreen();
                }
                break;
            case VK_F4:
                if (alt) ::PostQuitMessage(0);
                break;
            }
        }
        break;
        // The default window procedure will play a system notification sound 
        // when pressing the Alt+Enter keyboard combination if this message is 
        // not handled.
        case WM_SYSCHAR:
            break;
        case WM_SIZE:
            window->Resize();
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            break;
        default:
            return ::DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }
    else
    {
        return ::DefWindowProcW(hwnd, message, wParam, lParam);
    }

    return 0;
}

void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName)
{
    // Register a window class for creating our render window with.
    WNDCLASSEXW windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &WndProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = hInst;
    windowClass.hIcon = ::LoadIcon(hInst, 0);
    windowClass.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    windowClass.lpszMenuName = nullptr;
    windowClass.lpszClassName = windowClassName;
    windowClass.hIconSm = ::LoadIcon(hInst, nullptr);

    static ATOM atom = ::RegisterClassExW(&windowClass);
    assert(atom > 0);
}

HWND CreateWindow(const wchar_t* windowClassName, HINSTANCE hInst,
    const wchar_t* windowTitle, uint32_t width, uint32_t height)
{
    int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    // Center the window within the screen. Clamp to 0, 0 for the top-left corner.
    int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
    int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

    HWND hWnd = ::CreateWindowExW(
        0,
        windowClassName,
        windowTitle,
        WS_OVERLAPPEDWINDOW,
        windowX,
        windowY,
        windowWidth,
        windowHeight,
        nullptr,
        nullptr,
        hInst,
        nullptr
    );

    assert(hWnd && "Failed to create window");
    return hWnd;
}
