#pragma once

#include "Helpers.h"

class COMMAND_QUEUE;

class WINDOW
{
public:
	WINDOW(HINSTANCE hInstance);
	~WINDOW() { ; }

	void ParseCommandLineArguments();

	void CreateSwapChain(ComPtr<ID3D12CommandQueue> commandQueue);

	void Resize();
	void SwitchFullscreen();
	void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<ID3D12DescriptorHeap> descriptorHeap);

	inline void SetIsInitialized() { _isInitialized = true; }
	inline void SwitchVSync() { _vSync = !_vSync; };
	inline bool GetVSync() const { return _vSync; };
	inline bool GetTearingSupported() const { return _tearingSupported; };
	inline bool isInitialized() const { return _isInitialized; }
	inline bool GetIsWarp() const { return _useWarp; }

	inline UINT& GetCurrentBackBufferIndex() { return _currentBackBufferIndex; }
	inline ComPtr<ID3D12Resource> GetCurrentBackBuffer() const { return _backBuffers[_currentBackBufferIndex]; }
	inline ComPtr<IDXGISwapChain4> GetSwapChain() const { return _swapChain; }
	inline uint64_t& GetCurrentFrameFenceValue() { return _frameFenceValues[_currentBackBufferIndex]; }

	inline HWND GetWindowHandle() { return _hWnd; }

private:
	// Window handle.
	HWND _hWnd;
	RECT _windowRect;

	// Use WARP adapter
	bool _useWarp = false;
	bool _fullscreen = false;
	uint32_t _clientWidth = 1280;
	uint32_t _clientHeight = 720;

	bool _vSync = true;
	bool _tearingSupported = false;

	// Set to true once the DX12 objects have been initialized.
	bool _isInitialized = false;

	// DirectX12 objects
	ComPtr<IDXGISwapChain4>	_swapChain;
	ComPtr<ID3D12Resource>	_backBuffers[g_numFrames];
	UINT					_currentBackBufferIndex = 0u;

	// Synchronization objects
	uint64_t	_frameFenceValues[g_numFrames] = {};
};