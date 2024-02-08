#include "../Common.h"
#include "GraphicsCommon.h"
#include "../Geometry/Model.h"
#include "ShadowMap.h"

namespace Core
{
	void ShadowMap::Initialize(ID3D11Device* pDevice)
	{
		D3D11_TEXTURE2D_DESC desc = { 0, };
		desc.Width = m_ShadowWidth;
		desc.Height = m_ShadowHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_D32_FLOAT; // DSV 포맷 기준으로 설정해야 함.
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		
		m_ShadowBuffer.Initialize(pDevice, desc);
		m_ShadowConstantsBuffer.Initialize(pDevice);
	}

	void ShadowMap::Update(ID3D11DeviceContext* pContext, const Vector3& POS, const Matrix& VIEW, const Matrix& PROJECTION)
	{
		m_ShadowConstantsBuffer.CPU.EyeWorld = POS;
		m_ShadowConstantsBuffer.CPU.View = VIEW.Transpose();
		m_ShadowConstantsBuffer.CPU.Projection = PROJECTION.Transpose();
		m_ShadowConstantsBuffer.CPU.InverseProjection = PROJECTION.Invert().Transpose();
		m_ShadowConstantsBuffer.CPU.ViewProjection = (VIEW * PROJECTION).Transpose();

		m_ShadowConstantsBuffer.Upload(pContext);
	}

	void ShadowMap::Render(ID3D11DeviceContext* pContext, std::vector<Geometry::Model*>& pBasicList, Geometry::Model* pMirror)
	{
		ID3D11DepthStencilView* pShadowDSV = m_ShadowBuffer.GetDSV();

		setShadowViewport(pContext);
		pContext->OMSetRenderTargets(0, nullptr, pShadowDSV);
		pContext->ClearDepthStencilView(pShadowDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
		pContext->VSSetConstantBuffers(0, 1, &(m_ShadowConstantsBuffer.pGPU));
		pContext->PSSetConstantBuffers(0, 1, &(m_ShadowConstantsBuffer.pGPU));
		pContext->GSSetConstantBuffers(0, 1, &(m_ShadowConstantsBuffer.pGPU));

		for (size_t i = 0, size = pBasicList.size(); i < size; ++i)
		{
			Geometry::Model* const pCurModel = pBasicList[i];
			if (pCurModel->bCastShadow && pCurModel->bIsVisible)
			{
				setPipelineState(pContext, pCurModel->GetDepthOnlyPSO());
				pCurModel->Render(pContext);
			}
		}

		if (pMirror && pMirror->bCastShadow)
		{
			pMirror->Render(pContext);
		}
	}

	void ShadowMap::Destroy()
	{
		m_ShadowBuffer.Destroy();
		m_ShadowConstantsBuffer.Destroy();
	}

	void ShadowMap::setPipelineState(ID3D11DeviceContext* pContext, const Graphics::GraphicsPSO& PSO)
	{
		pContext->VSSetShader(PSO.pVertexShader, nullptr, 0);
		pContext->PSSetShader(PSO.pPixelShader, nullptr, 0);
		pContext->HSSetShader(PSO.pHullShader, nullptr, 0);
		pContext->DSSetShader(PSO.pDomainShader, nullptr, 0);
		pContext->GSSetShader(PSO.pGeometryShader, nullptr, 0);
		pContext->CSSetShader(nullptr, nullptr, 0);
		pContext->IASetInputLayout(PSO.pInputLayout);
		pContext->RSSetState(PSO.pRasterizerState);
		pContext->OMSetBlendState(PSO.pBlendState, PSO.BlendFactor, 0xffffffff);
		pContext->OMSetDepthStencilState(PSO.pDepthStencilState, PSO.StencilRef);
		pContext->IASetPrimitiveTopology(PSO.PrimitiveTopology);
	}

	void ShadowMap::setShadowViewport(ID3D11DeviceContext* pContext)
	{
		D3D11_VIEWPORT shadowViewport = { 0, };
		shadowViewport.TopLeftX = 0;
		shadowViewport.TopLeftY = 0;
		shadowViewport.Width = (float)m_ShadowWidth;
		shadowViewport.Height = (float)m_ShadowHeight;
		shadowViewport.MinDepth = 0.0f;
		shadowViewport.MaxDepth = 1.0f;

		pContext->RSSetViewports(1, &shadowViewport);
	}
}
