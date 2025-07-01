#pragma once

#include "Window.h"

// DirectX 12 specific and extension headers 
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3dx12.h>

class APPLICATION
{
public:
	static APPLICATION* CreateInstance(HINSTANCE hInstance);
	static void			DeleteInstance();
	static APPLICATION* Instance();

	inline WINDOW* GetWindow() const { return _windowInst; }
	inline ComPtr<ID3D12CommandQueue> GetCommandQueue() const { return _commandQueue; }
	inline ComPtr<ID3D12Device2> GetDevice() const { return _device; }

	void Update();
	void Render();

	void Flush();

private:
	APPLICATION(HINSTANCE hInstance);
	~APPLICATION();

	// Application Instance
	static APPLICATION* g_application;

	// Application Window
	WINDOW* _windowInst = nullptr;

	// DirectX12 objects
	ComPtr<ID3D12Device2>				_device;
	ComPtr<ID3D12CommandQueue>			_commandQueue;
	ComPtr<ID3D12GraphicsCommandList>	_commandList;
	ComPtr<ID3D12CommandAllocator>		_commandAllocators[g_numFrames];
	ComPtr<ID3D12DescriptorHeap>		_rtvDescriptorHeap;
	UINT								_rtvDescriptorSize = 0u;

	// Synchronization objects
	ComPtr<ID3D12Fence> _fence;
	uint64_t	_fenceValue = 0;
	HANDLE		_fenceEvent;
};