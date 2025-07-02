#pragma once

#include "Helpers.h"

class WINDOW;

class APPLICATION
{
public:
	static APPLICATION* CreateInstance(HINSTANCE hInstance);
	static void			DeleteInstance();
	static APPLICATION* Instance();

	inline WINDOW* GetWindow() { return _windowInst; }
	inline ComPtr<ID3D12CommandQueue> GetCommandQueue() { return _commandQueue; }
	inline ComPtr<ID3D12Device2> GetDevice() { return _device; }
	inline ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() { return _rtvDescriptorHeap; }

	void Update();
	void Render();
	void Flush();

private:
	APPLICATION(HINSTANCE hInstance);
	~APPLICATION();

	uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue);
	void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent,
		std::chrono::milliseconds duration = std::chrono::milliseconds::max());

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