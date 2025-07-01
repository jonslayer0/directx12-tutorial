#include "Application.h"

APPLICATION* APPLICATION::g_application = nullptr;

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

ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

    return d3d12CommandQueue;
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

ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device,
    D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;

    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device,
    D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

    return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device,
    ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

    ThrowIfFailed(commandList->Close());

    return commandList;
}

ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device)
{
    ComPtr<ID3D12Fence> fence;
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

    return fence;
}

HANDLE CreateEventHandle()
{
    HANDLE fenceEvent;
    fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    assert(fenceEvent && "Failed to create fence event.");

    return fenceEvent;
}

APPLICATION::APPLICATION(HINSTANCE hInstance)
{
    ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(false); //_useWarp

    _device = CreateDevice(dxgiAdapter4);
    _commandQueue = CreateCommandQueue(_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    _rtvDescriptorHeap = CreateDescriptorHeap(_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, g_numFrames);
    _rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    _windowInst = new WINDOW(hInstance);

    _windowInst->UpdateRenderTargetViews(_rtvDescriptorHeap);

    for (int i = 0; i < g_numFrames; ++i)
    {
        _commandAllocators[i] = CreateCommandAllocator(_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
    _commandList = CreateCommandList(_device, _commandAllocators[_windowInst->GetCurrentBackBufferIndex()], 
        D3D12_COMMAND_LIST_TYPE_DIRECT);

    _fence = CreateFence(_device);
    _fenceEvent = CreateEventHandle();

    _windowInst->SetIsInitialized();

    // TODO Message Loop
}

APPLICATION::~APPLICATION()
{
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

    auto commandAllocator = _commandAllocators[currentBackBufferIndex];

    commandAllocator->Reset();
    _commandList->Reset(commandAllocator.Get(), nullptr);

    // Clear the render target.
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        _commandList->ResourceBarrier(1, &barrier);

        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            currentBackBufferIndex, _rtvDescriptorSize);

        _commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }

    // Present
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        _commandList->ResourceBarrier(1, &barrier);

        ThrowIfFailed(_commandList->Close());

        ID3D12CommandList* const commandLists[] = {
            _commandList.Get()
        };
        _commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

        UINT syncInterval = _windowInst->GetVSync() ? 1 : 0;
        UINT presentFlags = _windowInst->GetTearingSupported() && !_windowInst->GetVSync() ? DXGI_PRESENT_ALLOW_TEARING : 0;
        ThrowIfFailed(_windowInst->GetSwapChain()->Present(syncInterval, presentFlags));

        _windowInst->GetCurrentFrameFenceValue() = Signal(_commandQueue, _fence, _fenceValue);

        currentBackBufferIndex = _windowInst->GetSwapChain()->GetCurrentBackBufferIndex();

        WaitForFenceValue(_fence, _windowInst->GetCurrentFrameFenceValue(), _fenceEvent);
    }
}

uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence,
    uint64_t& fenceValue)
{
    uint64_t fenceValueForSignal = ++fenceValue;
    ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValueForSignal));

    return fenceValueForSignal;
}

void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent,
    std::chrono::milliseconds duration = std::chrono::milliseconds::max())
{
    if (fence->GetCompletedValue() < fenceValue)
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
    }
}

void APPLICATION::Flush()
{
    uint64_t fenceValueForSignal = Signal(_commandQueue, _fence, _fenceValue);
    WaitForFenceValue(_fence, fenceValueForSignal, _fenceEvent);
}