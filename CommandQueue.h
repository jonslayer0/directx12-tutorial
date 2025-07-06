#pragma once

#include "Helpers.h"

#include <queue>
using namespace std;

class COMMAND_QUEUE
{
public:
	COMMAND_QUEUE(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	virtual ~COMMAND_QUEUE();

	ComPtr<ID3D12GraphicsCommandList2> GetCommadList();
	
	uint64_t ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2> commandList);
	
	uint64_t Signal();
	bool IsFenceComplete(uint64_t fenceValue);
	void WaitForFenceValue(uint64_t fenceValue, std::chrono::milliseconds duration = std::chrono::milliseconds::max());
	void Flush();

	ComPtr<ID3D12CommandQueue> GetCommandQueue() const;

private:
	ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
	ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator);

	struct COMMAND_ALLOCATOR_ENTRY
	{
		uint64_t fenceValue;
		ComPtr<ID3D12CommandAllocator> commandAllocator;
	};

	D3D12_COMMAND_LIST_TYPE		_CommandListType;
	ComPtr<ID3D12Device2>		_d3d12Device;
	ComPtr<ID3D12CommandQueue>	_d3d12CommandQueue;
	ComPtr<ID3D12Fence>			_d3d12Fence;
	HANDLE						_FenceEvent;
	uint64_t					_FenceValue = 0;

	queue<COMMAND_ALLOCATOR_ENTRY>				_CommandAllocatorQueue;
	queue<ComPtr<ID3D12GraphicsCommandList2>>	_CommandListQueue;
};