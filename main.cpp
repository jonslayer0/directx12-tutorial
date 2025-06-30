#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h> // For CommandLineToArgvW
#include <stdio.h>

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif
#if defined(max)
#undef max
#endif

// In order to define a function called CreateWindow, the Windows macro needs to
// be undefined.
#if defined(CreateWindow)
#undef CreateWindow
#endif

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12 specific and extension headers 
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3dx12.h>

// STL Headers
#include <algorithm>
#include <cassert>
#include <chrono>

// Helper functions
#include <Helpers.h>

// The number of swap chain back buffers.
const uint8_t g_NumFrames = 3;

// Use WARP adapter
bool g_UseWarp = false;
uint32_t g_ClientWidth = 1280;
uint32_t g_ClientHeight = 720;

// Set to true once the DX12 objects have been initialized.
bool g_IsInitialized = false;

// Window handle.
HWND g_hWnd;
RECT g_WindowRect;

// DirectX 12 Objects
ComPtr<ID3D12Device2>				g_Device;
ComPtr<ID3D12CommandQueue>			g_CommandQueue;
ComPtr<IDXGISwapChain4>				g_SwapChain;
ComPtr<ID3D12Resource>				g_BackBuffers[g_NumFrames];
ComPtr<ID3D12GraphicsCommandList>	g_CommandList;
ComPtr<ID3D12CommandAllocator>		g_CommandAllocators[g_NumFrames];
ComPtr<ID3D12DescriptorHeap>		g_RTVDescriptorHeap;

UINT g_RTVDescriptorSize = 0u;
UINT g_CurrentBackBufferIndex = 0u;

int main()
{
	printf("Hello World");
	return 0;
}