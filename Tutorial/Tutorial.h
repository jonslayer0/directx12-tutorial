#pragma once

#include "../Game.h"
#include "../Window.h"

#include <DirectXMath.h>

class TUTORIAL : public GAME
{
public:
	using super = GAME;

	TUTORIAL(const wstring& name, int width, int height, bool vSync);

	virtual bool LoadContent() override;
	virtual void UnloadContent() override;

protected:
	virtual void OnUpdate(UpdateEventArgs& e) override { ; }
	virtual void OnRender(RenderEventArgs& e) override { ; }
	virtual void OnKeyPressed(KeyEventArgs& e) override { ; }
	virtual void OnMouseWheel(MouseWheelEventArgs& e) override { ; }
	virtual void OnResize(ResizeEventArgs& e) override { ; }

private:
	void TransitionResource(ComPtr<ID3D12GraphicsCommandList2> commandList,
		ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES previousState,
		D3D12_RESOURCE_STATES afterState);

	void ClearRTV(ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE rtv, 
		FLOAT* clearColor);

	void ClearDepth(ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE dsv,
		FLOAT depth = 1.0f);

	void UpdateBufferResource(ComPtr<ID3D12GraphicsCommandList2> commandList,
		ID3D12Resource** pDestinationResource, 
		ID3D12Resource** pIntermediateResource,
		size_t numElements,
		size_t elementSize,
		const void* pBufferData,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	void ResizeDepthBuffer(int width, int height);

	uint64_t _FenceValues[g_numFrames] = {0};

	ComPtr<ID3D12Resource> _vertexBuffer;
	ComPtr<ID3D12Resource> _indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW _indexBufferView;

	ComPtr<ID3D12Resource> _depthBuffer;
	ComPtr<ID3D12DescriptorHeap> _dsvHeap;

	ComPtr<ID3D12RootSignature> _rootSignature;
	ComPtr<ID3D12PipelineState> _pipelineState;

	D3D12_VIEWPORT _viewport;
	D3D12_RECT _scissorRect;

	FLOAT _fov = 45.0f;
	DirectX::XMMATRIX _modelMatrix = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX _viewMatrix = DirectX::XMMatrixIdentity();;
	DirectX::XMMATRIX _projectionMatrix = DirectX::XMMatrixIdentity();;

	bool _contentLoaded = false;
};