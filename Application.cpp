#include "Application.h"
#include "Window.h"
#include "CommandQueue.h"

// STL Headers
#include <algorithm>
#include <cassert>
#include <chrono>

APPLICATION* APPLICATION::g_application = nullptr;

// DirectX12 initiliazing function headers
ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter);
ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);
ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

APPLICATION::APPLICATION(HINSTANCE hInstance)
{
    // Create Window
    _windowInst = new WINDOW(hInstance);

    // Get GPU Adapter
    ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(_windowInst->GetIsWarp());

    _device = CreateDevice(dxgiAdapter4);
    
    _commandQueue = new COMMAND_QUEUE(_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    
    _windowInst->CreateSwapChain(_commandQueue->GetCommandQueue());
    
    _rtvDescriptorHeap = CreateDescriptorHeap(_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, g_numFrames);
    _rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    _windowInst->UpdateRenderTargetViews(_device, _rtvDescriptorHeap);

    _windowInst->SetIsInitialized();
}

APPLICATION::~APPLICATION()
{
    delete _commandQueue;
    delete _windowInst;
}

APPLICATION* APPLICATION::CreateInstance(HINSTANCE hInstance) 
{ 
	if (g_application == nullptr) 
	{
		g_application = new APPLICATION(hInstance);
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
        wchar_t buffer[500];
        auto fps = frameCounter / elapsedSeconds;
        swprintf_s(&buffer[0], 500, L"FPS: %f\n", fps);
        OutputDebugString(buffer);

        frameCounter = 0;
        elapsedSeconds = 0.0;
    }
}

void APPLICATION::Render()
{
    UINT& currentBackBufferIndex = _windowInst->GetCurrentBackBufferIndex();
    auto backBuffer = _windowInst->GetCurrentBackBuffer();

    ComPtr<ID3D12GraphicsCommandList2> commandList = _commandQueue->GetCommandList();

    // Clear the render target.
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        commandList.Get()->ResourceBarrier(1, &barrier);

        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            currentBackBufferIndex, _rtvDescriptorSize);

        commandList.Get()->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }

    // Present
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList.Get()->ResourceBarrier(1, &barrier);

        _commandQueue->ExecuteCommandList(commandList);

        UINT syncInterval = _windowInst->GetVSync() ? 1 : 0;
        UINT presentFlags = _windowInst->GetTearingSupported() && !_windowInst->GetVSync() ? DXGI_PRESENT_ALLOW_TEARING : 0;
        ThrowIfFailed(_windowInst->GetSwapChain()->Present(syncInterval, presentFlags));

        _windowInst->GetCurrentFrameFenceValue() = _commandQueue->Signal();

        currentBackBufferIndex = _windowInst->GetSwapChain()->GetCurrentBackBufferIndex();

        _commandQueue->WaitForFenceValue(_windowInst->GetCurrentFrameFenceValue());
    }
}

void APPLICATION::Flush()
{
    _commandQueue->Flush(); // TODO : remove application Flush
}

void APPLICATION::Run()
{
    ::ShowWindow(_windowInst->GetWindowHandle(), SW_SHOW);

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
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

ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;

    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

COMMAND_QUEUE* APPLICATION::GetCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType)
{
    return new COMMAND_QUEUE(_device, commandListType); // todo track ?
}