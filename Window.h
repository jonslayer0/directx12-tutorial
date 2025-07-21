#pragma once

#include "Helpers.h"
#include "Events.h"
#include "HighResolutionClock.h"

#include <string.h>
using namespace std;

class GAME;

class WINDOW
{
public:
	WINDOW(const wstring& name, int width, int height, bool vSync);
	~WINDOW() { ; }

	void CreateSwapChain(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandQueue> commandQueue);

	void SwitchFullscreen();
	UINT Present();
	void Show();
	void Hide();


	inline void SetIsInitialized() { _isInitialized = true; }
	inline void SwitchVSync() { _vSync = !_vSync; };
	inline bool GetVSync() const { return _vSync; };
	inline bool GetTearingSupported() const { return _tearingSupported; };
	inline bool isInitialized() const { return _isInitialized; }
	inline bool GetIsWarp() const { return _useWarp; }
	inline HWND GetWindowHandle() const { return _hWnd; }

	inline UINT& GetCurrentBackBufferIndex() { return _currentBackBufferIndex; }
	inline ComPtr<ID3D12Resource> GetCurrentBackBuffer() const { return _backBuffers[_currentBackBufferIndex]; }
	inline ComPtr<IDXGISwapChain4> GetSwapChain() const { return _swapChain; }
	inline uint64_t& GetCurrentFrameFenceValue() { return _frameFenceValues[_currentBackBufferIndex]; }

	void UpdateRenderTargetViews();
	
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView();

	inline void RegisterCallbacks(std::shared_ptr<GAME> pGame) { _pGame = pGame; };

	// Update and Draw can only be called by the application.
	virtual void OnUpdate(UpdateEventArgs& e);
	virtual void OnRender(RenderEventArgs& e);

	// A keyboard key was pressed
	virtual void OnKeyPressed(KeyEventArgs& e);
	// A keyboard key was released
	virtual void OnKeyReleased(KeyEventArgs& e);

	// The mouse was moved
	virtual void OnMouseMoved(MouseMotionEventArgs& e);
	// A button on the mouse was pressed
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& e);
	// A button on the mouse was released
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& e);
	// The mouse wheel was moved.
	virtual void OnMouseWheel(MouseWheelEventArgs& e);

	// The window was resized.
	virtual void OnResize(ResizeEventArgs& e);

protected:

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
	ComPtr<ID3D12DescriptorHeap> _rtvDescriptorHeap;
	ComPtr<ID3D12Resource>	_backBuffers[g_numFrames];

	UINT					_rtvDescriptorSize = 0u;
	UINT					_currentBackBufferIndex = 0u;

	// Synchronization objects
	uint64_t	_frameFenceValues[g_numFrames] = {};

	std::weak_ptr<GAME> _pGame;

	uint64_t _FrameCounter;
	HighResolutionClock _UpdateClock;
	HighResolutionClock _RenderClock;
};