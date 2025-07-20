#include "Tutorial.h"

#include "../Application.h"
#include "../CommandQueue.h"
#include "../Window.h"

using namespace DirectX;

struct VERTEX_POS_COLOR
{
    XMFLOAT3 Position;
    XMFLOAT3 Color;
};

struct PIPELINE_STREAM_STATE
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primtiveTopologyType;
    CD3DX12_PIPELINE_STATE_STREAM_VS vertexShader;
    CD3DX12_PIPELINE_STATE_STREAM_PS pixelShader;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsvFormat;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS renderTargetFormats;
};

static VERTEX_POS_COLOR g_Vertices[8] = {
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
    { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
    { XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
    { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
    { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
    { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
    { XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
    { XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

static WORD g_Indices[36] =
{
    0, 1, 2, 0, 2, 3,
    4, 6, 5, 4, 7, 6,
    4, 5, 1, 4, 1, 0,
    3, 2, 6, 3, 6, 7,
    1, 5, 6, 1, 6, 2,
    4, 0, 3, 4, 3, 7
};

TUTORIAL::TUTORIAL(const wstring& name, int width, int height, bool vSync):
    super(name, width, height, vSync),
    _scissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
    _viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
{

}

void TUTORIAL::TransitionResource(ComPtr<ID3D12GraphicsCommandList2> commandList,
    ComPtr<ID3D12Resource> resource,
    D3D12_RESOURCE_STATES previousState,
    D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), previousState, afterState);
    commandList.Get()->ResourceBarrier(1, &barrier);
}

void TUTORIAL::ClearRTV(ComPtr<ID3D12GraphicsCommandList2> commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE rtv,
    FLOAT* clearColor)
{
    commandList.Get()->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void TUTORIAL::ClearDepth(ComPtr<ID3D12GraphicsCommandList2> commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE dsv,
    FLOAT depth)
{
    commandList.Get()->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void TUTORIAL::UpdateBufferResource(ComPtr<ID3D12GraphicsCommandList2> commandList,
    ID3D12Resource** pDestinationResource,
    ID3D12Resource** pIntermediateResource,
    size_t numElements,
    size_t elementSize,
    const void* pBufferData,
    D3D12_RESOURCE_FLAGS flags)
{
    ComPtr<ID3D12Device2> device = APPLICATION::Instance()->GetDevice();
    size_t bufferSize = numElements * elementSize;

    if (pBufferData)
    {
        CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
        ThrowIfFailed(device->CreateCommittedResource(
            &heapProp,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(pDestinationResource)));

        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = pBufferData;
        subresourceData.RowPitch = bufferSize;
        subresourceData.SlicePitch = subresourceData.RowPitch;

        ::UpdateSubresources(commandList.Get(), *pDestinationResource, *pIntermediateResource, 0, 0, 1, &subresourceData);
    }    
}

bool TUTORIAL::LoadContent()
{
    ComPtr<ID3D12Device2> device = APPLICATION::Instance()->GetDevice();
    COMMAND_QUEUE* commandQueue = APPLICATION::Instance()->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    ComPtr<ID3D12GraphicsCommandList2> commandList = commandQueue->GetCommandList();

    // Upload vertex buffer
    ComPtr<ID3D12Resource> intermediateVertexBuffer;
    UpdateBufferResource(commandList.Get(), &_vertexBuffer, &intermediateVertexBuffer, _countof(g_Vertices), sizeof(VERTEX_POS_COLOR), g_Vertices);

    // Create vertex buffer view
    _vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
    _vertexBufferView.SizeInBytes = sizeof(g_Vertices);
    _vertexBufferView.StrideInBytes = sizeof(VERTEX_POS_COLOR);

    // Upload index buffer
    ComPtr<ID3D12Resource> intermediateIndexBuffer;
    UpdateBufferResource(commandList.Get(), &_indexBuffer, &intermediateIndexBuffer, _countof(g_Indices), sizeof(WORD), g_Indices);

    // Create index buffer view
    _indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
    _indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    _indexBufferView.SizeInBytes = sizeof(g_Indices);

    // Create Descriptor Heap for depth RT
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    dsvHeapDesc.NumDescriptors = 1;
    device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_dsvHeap));

    // Load vertex and pixel shader from compiled binary
    ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderBlob));

    // Input layout for vertex shader
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    // Check for root signature version
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_2;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    }
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Create Root Signature from serialized root signature description
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = 
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_PARAMETER1 rootParameters[1] = {};
    rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // number of 32 bits elements ==> '16' floats (size of MMATRIX / 4)

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription = {};
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC::Init_1_2(rootSignatureDescription, _countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
    ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&_rootSignature)));

    // Pipeline State Object
    D3D12_RT_FORMAT_ARRAY rtFormatArrays = {};
    rtFormatArrays.NumRenderTargets = 1;
    rtFormatArrays.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    PIPELINE_STREAM_STATE pipelineStateStream = {};
    pipelineStateStream.rootSignature = _rootSignature.Get();
    pipelineStateStream.inputLayout = { inputLayout, _countof(inputLayout) };
    pipelineStateStream.primtiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.vertexShader = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.pixelShader = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.dsvFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.renderTargetFormats = rtFormatArrays;

    D3D12_PIPELINE_STATE_STREAM_DESC psoDesc = {};
    psoDesc.pPipelineStateSubobjectStream = &pipelineStateStream;
    psoDesc.SizeInBytes = sizeof(PIPELINE_STREAM_STATE);
    ThrowIfFailed(device->CreatePipelineState(&psoDesc, IID_PPV_ARGS(&_pipelineState)));

    uint64_t fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);

    _contentLoaded = true;

    ResizeDepthBuffer(GetClientWidth(), GetClientHeight());

    return true;
}

void TUTORIAL::UnloadContent()
{

}

void TUTORIAL::ResizeDepthBuffer(int width, int height)
{
    if (_contentLoaded)
    {
        APPLICATION::Instance()->Flush();

        width = std::max(1, width);
        height = std::max(1, height);

        ComPtr<ID3D12Device2> device = APPLICATION::Instance()->GetDevice();

        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        optimizedClearValue.DepthStencil = { 1.0f, 0 };

        CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        ThrowIfFailed(device->CreateCommittedResource(
            &heapProp,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &optimizedClearValue,
            IID_PPV_ARGS(&_depthBuffer))
        );

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

        device->CreateDepthStencilView(_depthBuffer.Get(), &dsvDesc, _dsvHeap->GetCPUDescriptorHandleForHeapStart());
    }
}

void TUTORIAL::OnUpdate(UpdateEventArgs& e)
{
    static uint64_t frameCount = 0;
    static double totalTime = 0.0;
    
    super::OnUpdate(e);

    totalTime = e.elapseTime;
    frameCount++;

    // Display FPS after every second
    if (totalTime > 1.0)
    {
        double fps = frameCount / totalTime;

        char buffer[512] = {};
        sprintf_s(buffer, "FPS : %f\n", fps);
        OutputDebugStringA(buffer);

        frameCount = 0;
        totalTime = 0.0;
    }

    float angle = static_cast<float>(e.totalTime * 90.0);
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
    _modelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

    const XMVECTOR eyePosition = XMVectorSet(0,0,-10,1);
    const XMVECTOR focusPoint = XMVectorSet(0,0,0,1);
    const XMVECTOR upDirection = XMVectorSet(0,1,0,0);
    _viewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    float aspectRatio = GetClientWidth() / static_cast<float>(GetClientHeight());
    _projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(_fov), aspectRatio, 0.1f, 100.0f);
}

void TUTORIAL::OnRender(RenderEventArgs& e)
{
    ComPtr<ID3D12Device2> device = APPLICATION::Instance()->GetDevice();
    COMMAND_QUEUE* commandQueue = APPLICATION::Instance()->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();
    
    UINT currentBackBufferIndex = _window->GetCurrentBackBufferIndex();
    auto backBuffer = _window->GetCurrentBackBuffer();
    auto rtv = _window->GetCurrentRenderTargetView(APPLICATION::Instance()->GetDescriptorHeapSize(), APPLICATION::Instance()->GetDescriptorHeap());
    auto dsv = _dsvHeap->GetCPUDescriptorHandleForHeapStart();

    // Clear back and depth
    {
        TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        ClearRTV(commandList, rtv, clearColor);
        ClearDepth(commandQueue->GetCommandList(), dsv);
    }

    commandList->SetPipelineState(_pipelineState.Get());
    commandList->SetGraphicsRootSignature(_rootSignature.Get());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
    commandList->IASetIndexBuffer(&_indexBufferView);

    commandList->RSSetViewports(1, &_viewport);
    commandList->RSSetScissorRects(1, &_scissorRect);

    commandList->OMSetRenderTargets(1, &rtv, false, &dsv);

    XMMATRIX mvpMatrix = XMMatrixMultiply(_modelMatrix, _viewMatrix);
    mvpMatrix = XMMatrixMultiply(mvpMatrix, _projectionMatrix);
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

    commandList->DrawIndexedInstanced(_countof(g_Indices), 1, 0, 0, 0);

    {
        TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        _fenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);
        currentBackBufferIndex = _window->Present();
        commandQueue->WaitForFenceValue(_fenceValues[currentBackBufferIndex]);
    }
}

void TUTORIAL::OnKeyPressed(KeyEventArgs& e)
{

}

void TUTORIAL::OnMouseWheel(MouseWheelEventArgs& e)
{

}

void TUTORIAL::OnResize(ResizeEventArgs& e)
{
    if (e.width != GetClientWidth() || e.height != GetClientHeight())
    {
        super::OnResize(e);

        _viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(e.width), static_cast<float>(e.height));

        ResizeDepthBuffer(e.width, e.height);
    }
}