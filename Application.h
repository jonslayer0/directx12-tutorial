#pragma once

#include "Helpers.h"

#include <unordered_map>
using namespace std;

class WINDOW;
class COMMAND_QUEUE;

class APPLICATION
{
public:
	static APPLICATION* CreateInstance();
	static void			DeleteInstance();
	static APPLICATION* Instance();

	void CreateRenderWindow(const wstring& name, int width, int height, bool vSync);
	void ParseCommandLineArguments();
	void DestroyWindow();

	inline WINDOW* GetWindow() { return _windowInst; }
	inline COMMAND_QUEUE* GetCommandQueue() { return _commandQueue; }
	COMMAND_QUEUE* GetCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType);
	inline ComPtr<ID3D12Device2> GetDevice() { return _device; }

	void Update();
	void Flush();

	void Run();
	void Quit();


private:
	APPLICATION();
	~APPLICATION();

	// Application Instance
	static APPLICATION* g_application;

	// 
	WINDOW*			_windowInst = nullptr;
	COMMAND_QUEUE*	_commandQueue = nullptr; // TODO : fuse
	unordered_map<D3D12_COMMAND_LIST_TYPE, COMMAND_QUEUE*> _commandQueues;

	// DirectX12 objects
	ComPtr<ID3D12Device2>		 _device;

	//
	wstring _Name;
	int _width = 1;
	int _height = 1;
	bool _vSync = false;
	bool _useWarp = false;
};