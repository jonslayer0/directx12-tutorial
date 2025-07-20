#pragma once

#include "Helpers.h"

#include <unordered_map>
using namespace std;

class WINDOW;
class COMMAND_QUEUE;

class APPLICATION
{
public:
	static APPLICATION* CreateInstance(HINSTANCE hInstance);
	static void			DeleteInstance();
	static APPLICATION* Instance();

	inline WINDOW* GetWindow() { return _windowInst; }
	inline COMMAND_QUEUE* GetCommandQueue() { return _commandQueue; }
	COMMAND_QUEUE* GetCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType);
	inline ComPtr<ID3D12Device2> GetDevice() { return _device; }
	inline ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() { return _rtvDescriptorHeap; }
	inline UINT GetDescriptorHeapSize() const { return _rtvDescriptorSize; }

	void Update();
	void Render();
	void Flush();

	void Run();
	void Quit();

private:
	APPLICATION(HINSTANCE hInstance);
	~APPLICATION();

	// Application Instance
	static APPLICATION* g_application;

	// 
	WINDOW*			_windowInst = nullptr;
	COMMAND_QUEUE*	_commandQueue = nullptr; // TODO : fuse
	unordered_map<D3D12_COMMAND_LIST_TYPE, COMMAND_QUEUE*> _commandQueues;

	// DirectX12 objects
	ComPtr<ID3D12Device2>		 _device;
	ComPtr<ID3D12DescriptorHeap> _rtvDescriptorHeap;
	UINT						 _rtvDescriptorSize = 0u;
};