#include "CommandQueue.h"

COMMAND_QUEUE::COMMAND_QUEUE(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type) :
	_CommandListType(type),
	_d3d12Device(device)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = _CommandListType;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(_d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&_d3d12CommandQueue)));
	ThrowIfFailed(_d3d12Device->CreateFence(_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_d3d12Fence)));

	_FenceEvent = ::CreateEvent(nullptr, false, false, nullptr);
	assert(_FenceEvent & "Failed to create fence event handle.");
}

ComPtr<ID3D12CommandAllocator> COMMAND_QUEUE::CreateCommandAllocator()
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(_d3d12Device->CreateCommandAllocator(_CommandListType, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList2> COMMAND_QUEUE::CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator)
{
	ComPtr<ID3D12CommandList> commandList;
	ThrowIfFailed(_d3d12Device->CreateCommandList(0, _CommandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
}

COMMAND_QUEUE::~COMMAND_QUEUE()
{

}

ComPtr<ID3D12GraphicsCommandList2> COMMAND_QUEUE::GetCommadList()
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList2> commandList;

	if (_CommandAllocatorQueue.empty() == false && IsFenceComplete(_CommandAllocatorQueue.front().fenceValue))
	{
		commandAllocator = _CommandAllocatorQueue.front().commandAllocator;
		_CommandAllocatorQueue.pop();

		ThrowIfFailed(commandAllocator.Reset());
	}
	else
	{
		commandAllocator = CreateCommandAllocator();
	}

	if (_CommandListQueue.empty() == false)
	{
		commandList = _CommandListQueue.front();
		_CommandListQueue.pop();

		ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
	}
	else
	{
		commandList = CreateCommandList(commandAllocator);
	}
}

uint64_t COMMAND_QUEUE::ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2>)
{

}

uint64_t COMMAND_QUEUE::Signal()
{

}

bool COMMAND_QUEUE::IsFenceComplete(uint64_t fence)
{

}

void COMMAND_QUEUE::WaitForFenceValue(uint64_t fence)
{

}

void COMMAND_QUEUE::Flush()
{

}

ComPtr<ID3D12CommandQueue> COMMAND_QUEUE::GetCommandQueue() const
{

}