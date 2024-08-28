#pragma once


// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/PipelineState.h
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_graphics_pipeline_state_desc

// PipelineStateObject: 렌더링할 때 Context의 상태를 어떻게 설정해줄지 저장.

class GraphicsPSO
{
public:
	GraphicsPSO() = default;
	~GraphicsPSO() { Destroy(); }

	void Destroy()
	{
		pRasterizerState = nullptr;
		pDepthStencilState = nullptr;
		pBlendState = nullptr;

		pInputLayout = nullptr;
		pGeometryShader = nullptr;
		pDomainShader = nullptr;
		pHullShader = nullptr;
		pPixelShader = nullptr;
		pVertexShader = nullptr;
	}

	void operator=(const GraphicsPSO& RHS);
	void SetBlendFactor(const float BLEND_FACTOR[4]);

public:
	ID3D11VertexShader* pVertexShader = nullptr;
	ID3D11PixelShader* pPixelShader = nullptr;
	ID3D11HullShader* pHullShader = nullptr;
	ID3D11DomainShader* pDomainShader = nullptr;
	ID3D11GeometryShader* pGeometryShader = nullptr;
	ID3D11InputLayout* pInputLayout = nullptr;

	ID3D11BlendState* pBlendState = nullptr;
	ID3D11DepthStencilState* pDepthStencilState = nullptr;
	ID3D11RasterizerState* pRasterizerState = nullptr;

	float BlendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	UINT StencilRef = 0;

	D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};

class ComputePSO
{
public:
	~ComputePSO() { Destroy(); }

	void Destroy() { pComputeShader = nullptr; }

	inline void operator=(const ComputePSO& RHS) { pComputeShader = RHS.pComputeShader; };

public:
	ID3D11ComputeShader* pComputeShader = nullptr;
};