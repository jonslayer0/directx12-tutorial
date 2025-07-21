#include "Window.h"
#include "Application.h"
#include "CommandQueue.h"
#include "Game.h"

#include <unordered_map>

static unordered_map<HWND, WINDOW*> gs_Windows;

// Windows initiliazing function headers
void EnableDebugLayer();
bool CheckTearingSupport();
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName);
HWND CreateWindow(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle, uint32_t width, uint32_t height);
ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
MouseButtonEventArgs::MouseButton DecodeMouseButton(UINT messageID);

WINDOW::WINDOW(const wstring& name, int width, int height, bool vSync):
    _clientWidth(width),
    _clientHeight(height),
    _vSync(vSync)
{
    // Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
    // Using this awareness context allows the client area of the window 
    // to achieve 100% scaling while still allowing non-client window content to 
    // be rendered in a DPI sensitive fashion.
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    EnableDebugLayer();

    _tearingSupported = CheckTearingSupport();

    HINSTANCE hInstance = NULL;
    RegisterWindowClass(hInstance, name.data());
    _hWnd = CreateWindow(name.data(), hInstance, L"Learning DirectX 12", _clientWidth, _clientHeight);

    // Initialize the global window rect variable.
    ::GetWindowRect(_hWnd, &_windowRect);

    // Swap chain creation called by application
}

void WINDOW::CreateSwapChain(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandQueue> commandQueue)
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

    _rtvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, g_numFrames);
    _rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
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

void WINDOW::UpdateRenderTargetViews()
{
    auto device = APPLICATION::Instance()->GetDevice();

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    for (int i = 0; i < g_numFrames; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        ThrowIfFailed(_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

        _backBuffers[i] = backBuffer;

        rtvHandle.Offset(rtvDescriptorSize);
    }
}

UINT WINDOW::Present()
{
    UINT syncInterval = _vSync ? 1 : 0;
    UINT presentFlags = _tearingSupported && !_vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    ThrowIfFailed(_swapChain->Present(syncInterval, presentFlags));

    return _swapChain->GetCurrentBackBufferIndex();
}

D3D12_CPU_DESCRIPTOR_HANDLE WINDOW::GetCurrentRenderTargetView()
{
   return CD3DX12_CPU_DESCRIPTOR_HANDLE(_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), _currentBackBufferIndex, _rtvDescriptorSize);
}

void WINDOW::OnUpdate(UpdateEventArgs&)
{
    _UpdateClock.Tick();

    if (auto pGame = _pGame.lock())
    {
        _FrameCounter++;

        UpdateEventArgs updateEventArgs(_UpdateClock.GetDeltaSeconds(), _UpdateClock.GetTotalSeconds());
        pGame->OnUpdate(updateEventArgs);
    }
}

void WINDOW::OnRender(RenderEventArgs&)
{
    _RenderClock.Tick();

    if (auto pGame = _pGame.lock())
    {
        RenderEventArgs renderEventArgs(_RenderClock.GetDeltaSeconds(), _RenderClock.GetTotalSeconds());
        pGame->OnRender(renderEventArgs);
    }
}

void WINDOW::OnKeyPressed(KeyEventArgs& e)
{
    if (auto pGame = _pGame.lock())
    {
        pGame->OnKeyPressed(e);
    }
}

void WINDOW::OnKeyReleased(KeyEventArgs& e)
{
    if (auto pGame = _pGame.lock())
    {
        pGame->OnKeyReleased(e);
    }
}

// The mouse was moved
void WINDOW::OnMouseMoved(MouseMotionEventArgs& e)
{
    if (auto pGame = _pGame.lock())
    {
        pGame->OnMouseMoved(e);
    }
}

// A button on the mouse was pressed
void WINDOW::OnMouseButtonPressed(MouseButtonEventArgs& e)
{
    if (auto pGame = _pGame.lock())
    {
        pGame->OnMouseButtonPressed(e);
    }
}

// A button on the mouse was released
void WINDOW::OnMouseButtonReleased(MouseButtonEventArgs& e)
{
    if (auto pGame = _pGame.lock())
    {
        pGame->OnMouseButtonReleased(e);
    }
}

// The mouse wheel was moved.
void WINDOW::OnMouseWheel(MouseWheelEventArgs& e)
{
    if (auto pGame = _pGame.lock())
    {
        pGame->OnMouseWheel(e);
    }
}

void WINDOW::OnResize(ResizeEventArgs& e)
{
    // Update the client size.
    if (_clientWidth != e.Width || _clientHeight != e.Height)
    {
        _clientWidth = std::max(1, e.Width);
        _clientHeight = std::max(1, e.Height);

        APPLICATION::Instance()->Flush();

        for (int i = 0; i < g_numFrames; ++i)
        {
            _backBuffers[i].Reset();
        }

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        ThrowIfFailed(_swapChain->GetDesc(&swapChainDesc));
        ThrowIfFailed(_swapChain->ResizeBuffers(g_numFrames, _clientWidth,
            _clientHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

        _currentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();

        UpdateRenderTargetViews();
    }

    if (auto pGame = _pGame.lock())
    {
        pGame->OnResize(e);
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

ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;

    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

//LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//    APPLICATION* application = APPLICATION::Instance();
//
//    if (application && application->GetWindow() && application->GetWindow()->isInitialized())
//    {
//        WINDOW* window = application->GetWindow();
//
//        switch (message)
//        {
//        case WM_PAINT:
//            application->Update();
//            application->Render();
//            break;
//        case WM_SYSKEYDOWN:
//        case WM_KEYDOWN:
//        {
//            bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
//
//            switch (wParam)
//            {
//            case 'V':
//                window->SwitchVSync();
//                break;
//            case VK_ESCAPE:
//                ::PostQuitMessage(0);
//                break;
//            case VK_RETURN:
//                if (alt)
//                {
//            case VK_F11:
//                window->SwitchFullscreen();
//                }
//                break;
//            case VK_F4:
//                if (alt) ::PostQuitMessage(0);
//                break;
//            }
//        }
//        break;
//        // The default window procedure will play a system notification sound 
//        // when pressing the Alt+Enter keyboard combination if this message is 
//        // not handled.
//        case WM_SYSCHAR:
//            break;
//        case WM_SIZE:
//            window->Resize();
//            break;
//        case WM_DESTROY:
//            ::PostQuitMessage(0);
//            break;
//        default:
//            return ::DefWindowProcW(hwnd, message, wParam, lParam);
//        }
//    }
//    else
//    {
//        return ::DefWindowProcW(hwnd, message, wParam, lParam);
//    }
//
//    return 0;
//}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WINDOW* pWindow;
    {
        auto iter = gs_Windows.find(hwnd);
        if (iter != gs_Windows.end())
        {
            pWindow = iter->second;
        }
    }

    if (pWindow)
    {
        switch (message)
        {
        case WM_PAINT:
        {
            // Delta time will be filled in by the Window.
            UpdateEventArgs updateEventArgs(0.0f, 0.0f);
            pWindow->OnUpdate(updateEventArgs);
            RenderEventArgs renderEventArgs(0.0f, 0.0f);
            // Delta time will be filled in by the Window.
            pWindow->OnRender(renderEventArgs);
        }
        break;
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            MSG charMsg;
            // Get the Unicode character (UTF-16)
            unsigned int c = 0;
            // For printable characters, the next message will be WM_CHAR.
            // This message contains the character code we need to send the KeyPressed event.
            // Inspired by the SDL 1.2 implementation.
            if (PeekMessage(&charMsg, hwnd, 0, 0, PM_NOREMOVE) && charMsg.message == WM_CHAR)
            {
                GetMessage(&charMsg, hwnd, 0, 0);
                c = static_cast<unsigned int>(charMsg.wParam);
            }
            bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            KeyCode::Key key = (KeyCode::Key)wParam;
            unsigned int scanCode = (lParam & 0x00FF0000) >> 16;
            KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Pressed, shift, control, alt);
            pWindow->OnKeyPressed(keyEventArgs);
        }
        break;
        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            KeyCode::Key key = (KeyCode::Key)wParam;
            unsigned int c = 0;
            unsigned int scanCode = (lParam & 0x00FF0000) >> 16;

            // Determine which key was released by converting the key code and the scan code
            // to a printable character (if possible).
            // Inspired by the SDL 1.2 implementation.
            unsigned char keyboardState[256];
            GetKeyboardState(keyboardState);
            wchar_t translatedCharacters[4];
            if (int result = ToUnicodeEx(static_cast<UINT>(wParam), scanCode, keyboardState, translatedCharacters, 4, 0, NULL) > 0)
            {
                c = translatedCharacters[0];
            }

            KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Released, shift, control, alt);
            pWindow->OnKeyReleased(keyEventArgs);
        }
        break;
        // The default window procedure will play a system notification sound 
        // when pressing the Alt+Enter keyboard combination if this message is 
        // not handled.
        case WM_SYSCHAR:
            break;
        case WM_MOUSEMOVE:
        {
            bool lButton = (wParam & MK_LBUTTON) != 0;
            bool rButton = (wParam & MK_RBUTTON) != 0;
            bool mButton = (wParam & MK_MBUTTON) != 0;
            bool shift = (wParam & MK_SHIFT) != 0;
            bool control = (wParam & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            MouseMotionEventArgs mouseMotionEventArgs(lButton, mButton, rButton, control, shift, x, y);
            pWindow->OnMouseMoved(mouseMotionEventArgs);
        }
        break;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        {
            bool lButton = (wParam & MK_LBUTTON) != 0;
            bool rButton = (wParam & MK_RBUTTON) != 0;
            bool mButton = (wParam & MK_MBUTTON) != 0;
            bool shift = (wParam & MK_SHIFT) != 0;
            bool control = (wParam & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Pressed, lButton, mButton, rButton, control, shift, x, y);
            pWindow->OnMouseButtonPressed(mouseButtonEventArgs);
        }
        break;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        {
            bool lButton = (wParam & MK_LBUTTON) != 0;
            bool rButton = (wParam & MK_RBUTTON) != 0;
            bool mButton = (wParam & MK_MBUTTON) != 0;
            bool shift = (wParam & MK_SHIFT) != 0;
            bool control = (wParam & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Released, lButton, mButton, rButton, control, shift, x, y);
            pWindow->OnMouseButtonReleased(mouseButtonEventArgs);
        }
        break;
        case WM_MOUSEWHEEL:
        {
            // The distance the mouse wheel is rotated.
            // A positive value indicates the wheel was rotated to the right.
            // A negative value indicates the wheel was rotated to the left.
            float zDelta = ((int)(short)HIWORD(wParam)) / (float)WHEEL_DELTA;
            short keyStates = (short)LOWORD(wParam);

            bool lButton = (keyStates & MK_LBUTTON) != 0;
            bool rButton = (keyStates & MK_RBUTTON) != 0;
            bool mButton = (keyStates & MK_MBUTTON) != 0;
            bool shift = (keyStates & MK_SHIFT) != 0;
            bool control = (keyStates & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            // Convert the screen coordinates to client coordinates.
            POINT clientToScreenPoint;
            clientToScreenPoint.x = x;
            clientToScreenPoint.y = y;
            ScreenToClient(hwnd, &clientToScreenPoint);

            MouseWheelEventArgs mouseWheelEventArgs(zDelta, lButton, mButton, rButton, control, shift, (int)clientToScreenPoint.x, (int)clientToScreenPoint.y);
            pWindow->OnMouseWheel(mouseWheelEventArgs);
        }
        break;
        case WM_SIZE:
        {
            int width = ((int)(short)LOWORD(lParam));
            int height = ((int)(short)HIWORD(lParam));

            ResizeEventArgs resizeEventArgs(width, height);
            pWindow->OnResize(resizeEventArgs);
        }
        break;
        case WM_DESTROY:
        {
            // If a window is being destroyed, remove it from the 
            // window maps.
            RemoveWindow(hwnd);

            if (gs_Windows.empty())
            {
                // If there are no more windows, quit the application.
                PostQuitMessage(0);
            }
        }
        break;
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }
    else
    {
        return DefWindowProcW(hwnd, message, wParam, lParam);
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


// Convert the message ID into a MouseButton ID
MouseButtonEventArgs::MouseButton DecodeMouseButton(UINT messageID)
{
    MouseButtonEventArgs::MouseButton mouseButton = MouseButtonEventArgs::None;
    switch (messageID)
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    {
        mouseButton = MouseButtonEventArgs::Left;
    }
    break;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    {
        mouseButton = MouseButtonEventArgs::Right;
    }
    break;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    {
        mouseButton = MouseButtonEventArgs::Middel;
    }
    break;
    }

    return mouseButton;
}
