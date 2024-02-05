#include "../Common.h"
#include "PipelineState.h"

namespace Graphics
{
	void GraphicsPSO::operator=(const GraphicsPSO& RHS)
	{
		pVertexShader = RHS.pVertexShader;
		pPixelShader = RHS.pPixelShader;
		pHullShader = RHS.pHullShader;
		pDomainShader = RHS.pDomainShader;
		pGeometryShader = RHS.pGeometryShader;
		pInputLayout = RHS.pInputLayout;
		pBlendState = RHS.pBlendState;
		pDepthStencilState = RHS.pDepthStencilState;
		pRasterizerState = RHS.pRasterizerState;
		StencilRef = RHS.StencilRef;
		for (int i = 0; i < 4; ++i)
		{
			BlendFactor[i] = RHS.BlendFactor[i];
		}
		PrimitiveTopology = RHS.PrimitiveTopology;
	}

	void GraphicsPSO::SetBlendFactor(const float BLEND_FACTOR[4])
	{
		memcpy(BlendFactor, BLEND_FACTOR, sizeof(float) * 4);
	}
}
