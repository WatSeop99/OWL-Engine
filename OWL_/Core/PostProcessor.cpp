#include "../Common.h"
#include "ConstantBuffers.h"
#include "../Geometry/GeometryGenerator.h"
#include "GraphicsCommon.h"
#include "GraphicsUtils.h"
#include "../Geometry/MeshData.h"
#include "../Geometry/Model.h"
#include "PostProcessor.h"

namespace Core
{
	void PostProcessor::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const struct PostProcessingBuffers& CONFIG, const int WIDTH, const int HEIGHT, const int BLOOMLEVELS)
	{
		HRESULT hr = S_OK;

		destroy();

		m_ScreenWidth = WIDTH;
		m_ScreenHeight = HEIGHT;

		// ��ũ�� ���� ����.
		struct Geometry::MeshData meshData = INIT_MESH_DATA;
		Geometry::MakeSquare(&meshData);

		pMesh = (struct Geometry::Mesh*)Malloc(sizeof(struct Geometry::Mesh));
		*pMesh = INIT_MESH;

		hr = Graphics::CreateVertexBuffer(pDevice, meshData.Vertices, &(pMesh->pVertexBuffer));
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(pMesh->pVertexBuffer, "PostProcess::pMesh->pVertexBuffer");

		pMesh->IndexCount = (UINT)(meshData.Indices.size());
		hr = Graphics::CreateIndexBuffer(pDevice, meshData.Indices, &(pMesh->pIndexBuffer));
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(pMesh->pIndexBuffer, "PostProcess::pMesh->pIndexBuffer");

		// ��ó�� ȿ���� ���� ����.
		setRenderConfig(CONFIG);
		createPostBackBuffers(pDevice);

		// ��ó�� ȿ���� constBuffer.
		hr = Graphics::CreateConstBuffer(pDevice, PostEffectsConstsCPU, &m_pPostEffectsConstsGPU);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pPostEffectsConstsGPU, "PostProcessor::m_pPostEffectsConstsGPU");

		// Bloom Down/Up.
		m_pBloomSRVs.resize(BLOOMLEVELS);
		m_pBloomRTVs.resize(BLOOMLEVELS);
		for (int i = 0; i < BLOOMLEVELS; ++i)
		{
			int div = (int)pow(2, i);
			createImageResources(pDevice, pContext, WIDTH / div, HEIGHT / div, &m_pBloomSRVs[i], &m_pBloomRTVs[i]);
#ifdef _DEBUG
			std::string post(std::to_string(i) + "]");
			std::string debugStringSRV("PostProcess::m_pBloomSRVs[");
			std::string debugStringRTV("PostProcess::m_pBloomRTVs[");
			debugStringSRV += post;
			debugStringRTV += post;
			SET_DEBUG_INFO_TO_OBJECT(m_pBloomSRVs[i], debugStringSRV.c_str());
			SET_DEBUG_INFO_TO_OBJECT(m_pBloomRTVs[i], debugStringRTV.c_str());
#endif
		}

		m_pBloomDownFilters.resize(BLOOMLEVELS - 1);
		for (int i = 0; i < BLOOMLEVELS - 1; ++i)
		{
			int div = (int)pow(2, i + 1);
			m_pBloomDownFilters[i].Initialize(pDevice, pContext, Graphics::g_pBloomDownPS, WIDTH / div, HEIGHT / div);
			if (i == 0)
			{
				m_pBloomDownFilters[i].SetShaderResources({ m_pPostEffectsSRV });
			}
			else
			{
				m_pBloomDownFilters[i].SetShaderResources({ m_pBloomSRVs[i] });
			}

			m_pBloomDownFilters[i].SetRenderTargets({ m_pBloomRTVs[i + 1] });
		}

		m_pBloomUpFilters.resize(BLOOMLEVELS - 1);
		for (int i = 0; i < BLOOMLEVELS - 1; ++i)
		{
			int level = BLOOMLEVELS - 2 - i;
			int div = (int)pow(2, level);
			m_pBloomUpFilters[i].Initialize(pDevice, pContext, Graphics::g_pBloomUpPS, WIDTH / div, HEIGHT / div);
			m_pBloomUpFilters[i].SetShaderResources({ m_pBloomSRVs[level + 1] });
			m_pBloomUpFilters[i].SetRenderTargets({ m_pBloomRTVs[level] });
		}

		// combine + tone mapping.
		CombineFilter.Initialize(pDevice, pContext, Graphics::g_pCombinePS, WIDTH, HEIGHT);
		CombineFilter.SetShaderResources({ m_pPostEffectsSRV, m_pBloomSRVs[0], m_pPrevSRV }); // resource[1]�� ��� ������ ���� ���� ������ ���.
		CombineFilter.SetRenderTargets({ m_pBackBufferRTV });
		CombineFilter.ConstantsData.Strength = 0.0f; // bloom strength.
		CombineFilter.ConstantsData.Option1 = 1.0f;  // exposure.
		CombineFilter.ConstantsData.Option2 = 2.2f;  // gamma.

		// ����: float render target������ gamma correction ���� ����. (gamma = 1.0)
		Graphics::UpdateBuffer(pContext, PostEffectsConstsCPU, m_pPostEffectsConstsGPU);
		CombineFilter.UpdateConstantBuffers(pContext);
	}

	void PostProcessor::Update(ID3D11DeviceContext* pContext)
	{
		_ASSERT(m_pPostEffectsConstsGPU != nullptr);

		if (PostEffectsUpdateFlag > 0)
		{
			Graphics::UpdateBuffer(pContext, PostEffectsConstsCPU, m_pPostEffectsConstsGPU);
			PostEffectsUpdateFlag = 0;
		}

		if (CombineUpdateFlag > 0)
		{
			CombineFilter.UpdateConstantBuffers(pContext);
			CombineUpdateFlag = 0;
		}
	}

	void PostProcessor::Render(ID3D11DeviceContext* pContext)
	{
		// Resolve MSAA texture.
		pContext->ResolveSubresource(m_pResolvedBuffer, 0, m_pFloatBuffer, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);

		// ��ũ�� �������� ���� ���� ���ۿ� ���ؽ� ���۸� �̸� ����.
		UINT stride = sizeof(struct Geometry::Vertex);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &(pMesh->pVertexBuffer), &stride, &offset);
		pContext->IASetIndexBuffer(pMesh->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// post effecting �� post processing.
		renderPostEffects(pContext);
		renderPostProcessing(pContext);
	}

	void PostProcessor::createPostBackBuffers(ID3D11Device* pDevice)
	{
		// ��ó���� ���� back buffer.

		HRESULT hr = S_OK;
		D3D11_TEXTURE2D_DESC desc = { 0, };
		m_pBackBuffer->GetDesc(&desc);
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		hr = pDevice->CreateTexture2D(&desc, nullptr, &m_pPostEffectsBuffer);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pPostEffectsBuffer, "PostProcessor::m_pPostEffectsBuffer");

		hr = pDevice->CreateShaderResourceView(m_pPostEffectsBuffer, nullptr, &m_pPostEffectsSRV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pPostEffectsSRV, "PostProcessor::m_pPostEffectsSRV");

		hr = pDevice->CreateRenderTargetView(m_pPostEffectsBuffer, nullptr, &m_pPostEffectsRTV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pPostEffectsRTV, "PostProcessor::m_pPostEffectsRTV");
	}

	void PostProcessor::createImageResources(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, int width, int height, ID3D11ShaderResourceView** ppSrv, ID3D11RenderTargetView** ppRtv)
	{
		HRESULT hr = S_OK;

		ID3D11Texture2D* texture = nullptr;

		D3D11_TEXTURE2D_DESC txtDesc = { 0, };
		txtDesc.Width = width;
		txtDesc.Height = height;
		txtDesc.MipLevels = txtDesc.ArraySize = 1;
		txtDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // �̹��� ó���뵵.
		txtDesc.SampleDesc.Count = 1;
		txtDesc.Usage = D3D11_USAGE_DEFAULT;
		txtDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS;
		txtDesc.MiscFlags = 0;
		txtDesc.CPUAccessFlags = 0;

		hr = pDevice->CreateTexture2D(&txtDesc, nullptr, &texture);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(texture, "PostProcessor::texture");

		hr = pDevice->CreateRenderTargetView(texture, nullptr, ppRtv);
		BREAK_IF_FAILED(hr);

		hr = pDevice->CreateShaderResourceView(texture, nullptr, ppSrv);
		BREAK_IF_FAILED(hr);

		RELEASE(texture);
	}

	void PostProcessor::renderPostEffects(ID3D11DeviceContext* pContext)
	{
		// PostEffects (m_pGlobalConstsGPU ���).
		setViewport(pContext);
		setPipelineState(pContext, Graphics::g_PostEffectsPSO);
		setGlobalConsts(pContext , &m_pGlobalConstsGPU);

		ID3D11ShaderResourceView* ppPostEffectsSRVs[] = { m_pResolvedSRV, m_pDepthOnlySRV };
		UINT numPostEffectsSRVs = _countof(ppPostEffectsSRVs);
		pContext->PSSetShaderResources(20, numPostEffectsSRVs, ppPostEffectsSRVs);
		pContext->OMSetRenderTargets(1, &m_pPostEffectsRTV, nullptr);
		pContext->PSSetConstantBuffers(5, 1, &m_pPostEffectsConstsGPU);
		pContext->DrawIndexed(pMesh->IndexCount, 0, 0);

		ID3D11ShaderResourceView* ppNulls[2] = { nullptr, };
		pContext->PSSetShaderResources(20, 2, ppNulls);
	}

	void PostProcessor::renderPostProcessing(ID3D11DeviceContext* pContext)
	{
		// ��ó�� (���� ���� ���� �̹��� ó��).
		setPipelineState(pContext, Graphics::g_PostProcessingPSO);

		pContext->PSSetSamplers(0, 1, &Graphics::g_pLinearClampSS);

		// ������ �ʿ��� ��쿡�� ���.
		if (CombineFilter.ConstantsData.Strength > 0.0f)
		{
			for (size_t i = 0, size = m_pBloomDownFilters.size(); i < size; ++i)
			{
				renderImageFilter(pContext, m_pBloomDownFilters[i]);
			}
			for (size_t i = 0, size = m_pBloomUpFilters.size(); i < size; ++i)
			{
				renderImageFilter(pContext, m_pBloomUpFilters[i]);
			}
		}

		renderImageFilter(pContext, CombineFilter);

		pContext->CopyResource(m_pPrevBuffer, m_pBackBuffer); // ��� ���� ȿ���� ���� ������ ��� ����.
	}

	void PostProcessor::renderImageFilter(ID3D11DeviceContext* pContext, const ImageFilter& IMAGE_FILTER)
	{
		IMAGE_FILTER.Render(pContext);
		pContext->DrawIndexed(pMesh->IndexCount, 0, 0);
	}

	void PostProcessor::setViewport(ID3D11DeviceContext* pContext)
	{
		// Set the viewport
		m_Viewport = { 0, };
		m_Viewport.TopLeftX = 0;
		m_Viewport.TopLeftY = 0;
		m_Viewport.Width = (float)m_ScreenWidth;
		m_Viewport.Height = (float)m_ScreenHeight;
		m_Viewport.MinDepth = 0.0f;
		m_Viewport.MaxDepth = 1.0f;

		pContext->RSSetViewports(1, &m_Viewport);
	}

	void PostProcessor::setRenderConfig(const struct PostProcessingBuffers& CONFIG)
	{
		m_pGlobalConstsGPU = CONFIG.pGlobalConstsGPU;
		m_pBackBuffer = CONFIG.pBackBuffer;
		m_pFloatBuffer = CONFIG.pFloatBuffer;
		m_pResolvedBuffer = CONFIG.pResolvedBuffer;
		m_pPrevBuffer = CONFIG.pPrevBuffer;
		m_pBackBufferRTV = CONFIG.pBackBufferRTV;
		m_pResolvedSRV = CONFIG.pResolvedSRV;
		m_pPrevSRV = CONFIG.pPrevSRV;
		m_pDepthOnlySRV = CONFIG.pDepthOnlySRV;
	}

	void PostProcessor::setPipelineState(ID3D11DeviceContext* pContext, Graphics::GraphicsPSO& PSO)
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

	void PostProcessor::setGlobalConsts(ID3D11DeviceContext* pContext, ID3D11Buffer** ppGlobalConstsGPU)
	{
		// ���̴��� �ϰ��� ���� cbuffer GlobalConstants : register(b0).
		pContext->VSSetConstantBuffers(0, 1, ppGlobalConstsGPU);
		pContext->PSSetConstantBuffers(0, 1, ppGlobalConstsGPU);
		pContext->GSSetConstantBuffers(0, 1, ppGlobalConstsGPU);
	}

	void PostProcessor::destroy()
	{
		m_pGlobalConstsGPU = nullptr;
		m_pBackBuffer = nullptr;
		m_pFloatBuffer = nullptr;
		m_pResolvedBuffer = nullptr;
		m_pPrevBuffer = nullptr;
		m_pBackBufferRTV = nullptr;
		m_pResolvedSRV = nullptr;
		m_pPrevSRV = nullptr;
		m_pDepthOnlySRV = nullptr;

		for (size_t i = 0, size = m_pBloomSRVs.size(); i < size; ++i)
		{
			RELEASE(m_pBloomSRVs[i]);
			RELEASE(m_pBloomRTVs[i]);
		}
		m_pBloomSRVs.clear();
		m_pBloomRTVs.clear();
		m_pBloomDownFilters.clear();
		m_pBloomUpFilters.clear();

		SAFE_RELEASE(m_pPostEffectsConstsGPU);

		SAFE_RELEASE(m_pPostEffectsBuffer);
		SAFE_RELEASE(m_pPostEffectsRTV);
		SAFE_RELEASE(m_pPostEffectsSRV);

		if (pMesh)
		{
			ReleaseMesh(&pMesh);
		}
	}
}