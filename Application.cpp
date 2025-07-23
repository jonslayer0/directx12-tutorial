#include "Application.h"
#include "Window.h"
#include "CommandQueue.h"
#include "Game.h"

// STL Headers
#include <algorithm>
#include <cassert>
#include <chrono>

APPLICATION* APPLICATION::g_application = nullptr;

// DirectX12 initiliazing function headers
ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter);
ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);
APPLICATION::APPLICATION()
{
}

APPLICATION::~APPLICATION()
{
    delete _commandQueue;
}

APPLICATION* APPLICATION::CreateInstance(HINSTANCE hInstance)
{ 
	if (g_application == nullptr) 
	{
		g_application = new APPLICATION();
        g_application->_hInstance = hInstance;
	}
	return g_application; 
}

void APPLICATION::DeleteInstance()
{
	delete g_application;
    g_application = nullptr;
}

APPLICATION* APPLICATION::Instance() 
{ 
	return  g_application; 
}


WINDOW* APPLICATION::CreateRenderWindow(const wstring& name, int width, int height, bool vSync)
{
    // Create Window
    WINDOW* newWindow = new WINDOW(_hInstance, name, width, height, vSync);
    
    // Get GPU Adapter
    ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(newWindow->GetIsWarp());

    _device = CreateDevice(dxgiAdapter4);

    _commandQueue = new COMMAND_QUEUE(_device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    newWindow->CreateSwapChain(_device, _commandQueue->GetCommandQueue());
    newWindow->UpdateRenderTargetViews();
    newWindow->SetIsInitialized();

    WINDOW::gs_Windows.emplace(std::pair<HWND, WINDOW*>(newWindow->GetWindowHandle(), newWindow));

    return newWindow;
}

void APPLICATION::ParseCommandLineArguments()
{
    int argc;
    wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

    for (size_t i = 0; i < argc; ++i)
    {
        if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0)
        {
            _width = ::wcstol(argv[++i], nullptr, 10);
        }

        if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0)
        {
            _height = ::wcstol(argv[++i], nullptr, 10);
        }

        if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
        {
            _useWarp = true;
        }
    }

    // Free memory allocated by CommandLineToArgvW
    ::LocalFree(argv);
}

void APPLICATION::Update()
{
    static uint64_t frameCounter = 0;
    static double elapsedSeconds = 0.0;
    static std::chrono::high_resolution_clock clock;
    static auto t0 = clock.now();

    frameCounter++;
    auto t1 = clock.now();
    auto deltaTime = t1 - t0;
    t0 = t1;

    elapsedSeconds += deltaTime.count() * 1e-9;
    if (elapsedSeconds > 1.0)
    {
        wchar_t buffer[500] = {};
        auto fps = frameCounter / elapsedSeconds;
        swprintf_s(&buffer[0], 500, L"FPS: %f\n", fps);
        OutputDebugString(buffer);

        frameCounter = 0;
        elapsedSeconds = 0.0;
    }
}

void APPLICATION::Flush()
{
    _commandQueue->Flush();
    for (auto queueIt : _commandQueues)
    {
        queueIt.second->Flush();
    }
}

int APPLICATION::Run(std::shared_ptr<GAME> pGame)
{
    if (!pGame->Initialize()) return 1;
    if (!pGame->LoadContent()) return 2;

    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Flush any commands in the commands queues before quiting.
    Flush();

    pGame->UnloadContent();
    pGame->Destroy();

    return static_cast<int>(msg.wParam);
}

void APPLICATION::Quit()
{
    DeleteInstance();
}

ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter)
{
    ComPtr<ID3D12Device2> d3d12Device2;
    ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));

    // Enable debug messages in debug mode.
#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        // Suppress whole categories of messages
        //D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] =
        {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
        };

        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        //NewFilter.DenyList.NumCategories = _countof(Categories);
        //NewFilter.DenyList.pCategoryList = Categories;
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;

        ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
    }
#endif

    return d3d12Device2;
}

ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp)
{
    ComPtr<IDXGIFactory4> dxgiFactory;
    UINT createFactoryFlags = 0;

#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

    ComPtr<IDXGIAdapter1> dxgiAdapterTemp;  // Use to find best adapter
    ComPtr<IDXGIAdapter4> dxgiAdapterFinal; // Returned best adapter

    if (useWarp)
    {
        ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapterTemp)));
        ThrowIfFailed(dxgiAdapterTemp.As(&dxgiAdapterFinal));
    }
    else
    {
        SIZE_T maxDedicatedVideoMemory = 0;

        for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapterTemp) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
            dxgiAdapterTemp->GetDesc1(&dxgiAdapterDesc1);

            // Check to see if the adapter can create a D3D12 device without actually 
            // creating it. The adapter with the largest dedicated video memory is favored.

            if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
                SUCCEEDED(D3D12CreateDevice(dxgiAdapterTemp.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
                dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
            {
                maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                ThrowIfFailed(dxgiAdapterTemp.As(&dxgiAdapterFinal));
            }
        }
    }

    return dxgiAdapterFinal;
}

COMMAND_QUEUE* APPLICATION::GetCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType)
{
    auto it = _commandQueues.find(commandListType);
    if (it == _commandQueues.end())
    {
        _commandQueues[commandListType] = new COMMAND_QUEUE(_device, commandListType);
        return _commandQueues[commandListType];
    }
    return (it->second);
}