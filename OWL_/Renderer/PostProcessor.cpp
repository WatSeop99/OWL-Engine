#include "../Common.h"
#include "../Graphics/ConstantDataType.h"
#include "../Geometry/GeometryGenerator.h"
#include "../Graphics/GraphicsCommon.h"
#include "../Graphics/GraphicsUtils.h"
#include "../Geometry/Mesh.h"
#include "../Geometry/MeshInfo.h"
#include "../Geometry/Model.h"
#include "PostProcessor.h"


void PostProcessor::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const PostProcessingBuffers& CONFIG, const int WIDTH, const int HEIGHT, const int BLOOMLEVELS)
{
	_ASSERT(pDevice);
	_ASSERT(pContext);

	HRESULT hr = S_OK;

	m_ScreenWidth = WIDTH;
	m_ScreenHeight = HEIGHT;

	// 스크린 공간 설정.
	MeshInfo meshInfo;
	MakeSquare(&meshInfo);

	pMesh = New Mesh;
	pMesh->Initialize(pDevice, pContext);

	hr = CreateVertexBuffer(pDevice, meshInfo.Vertices, &pMesh->pVertexBuffer);
	BREAK_IF_FAILED(hr);

	pMesh->IndexCount = (UINT)meshInfo.Indices.size();
	hr = CreateIndexBuffer(pDevice, meshInfo.Indices, &pMesh->pIndexBuffer);
	BREAK_IF_FAILED(hr);

	// 후처리 효과용 버퍼 생성.
	setRenderConfig(CONFIG);
	createPostBackBuffers(pDevice);

	// 후처리 효과용 constBuffer.
	hr = CreateConstBuffer(pDevice, PostEffectsConstsCPU, &m_pPostEffectsConstsGPU);
	BREAK_IF_FAILED(hr);

	// Bloom Down/Up.
	m_pBloomSRVs.resize(BLOOMLEVELS);
	m_pBloomRTVs.resize(BLOOMLEVELS);
	for (int i = 0; i < BLOOMLEVELS; ++i)
	{
		int div = (int)pow(2, i);
		createImageResources(pDevice, pContext, WIDTH / div, HEIGHT / div, &m_pBloomSRVs[i], &m_pBloomRTVs[i]);
#ifdef _DEBUG
		char szDebugStringName1[256];
		char szDebugStringName2[256];
		sprintf_s(szDebugStringName1, 256, "PostProcess::m_pBloomSRVs[%d]", i);
		sprintf_s(szDebugStringName2, 256, "PostProcess::m_pBloomRTVs[%d]", i);

		SET_DEBUG_INFO_TO_OBJECT(m_pBloomSRVs[i], szDebugStringName1);
		SET_DEBUG_INFO_TO_OBJECT(m_pBloomRTVs[i], szDebugStringName2);
#endif
	}

	m_pBloomDownFilters.resize(BLOOMLEVELS - 1);
	for (int i = 0; i < BLOOMLEVELS - 1; ++i)
	{
		int div = (int)pow(2, i + 1);
		m_pBloomDownFilters[i].Initialize(pDevice, pContext, g_pBloomDownPS, WIDTH / div, HEIGHT / div);
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
		m_pBloomUpFilters[i].Initialize(pDevice, pContext, g_pBloomUpPS, WIDTH / div, HEIGHT / div);
		m_pBloomUpFilters[i].SetShaderResources({ m_pBloomSRVs[level + 1] });
		m_pBloomUpFilters[i].SetRenderTargets({ m_pBloomRTVs[level] });
	}

	// combine + tone mapping.
	CombineFilter.Initialize(pDevice, pContext, g_pCombinePS, WIDTH, HEIGHT);
	CombineFilter.SetShaderResources({ m_pPostEffectsSRV, m_pBloomSRVs[0], m_pPrevSRV }); // resource[1]은 모션 블러를 위한 이전 프레임 결과.
	CombineFilter.SetRenderTargets({ m_pBackBufferRTV });
	
	ImageFilterConstData* pCombineFilterConstData = (ImageFilterConstData*)CombineFilter.GetConstantBufferPtr()->pSystemMem;
	pCombineFilterConstData->Strength = 0.0f;	// bloom strength.
	pCombineFilterConstData->Option1 = 1.0f;	// exposure.
	pCombineFilterConstData->Option2 = 2.2f;	// gamma.

	// 주의: float render target에서는 gamma correction 하지 않음. (gamma = 1.0)
	UpdateBuffer(pContext, PostEffectsConstsCPU, m_pPostEffectsConstsGPU);
	CombineFilter.UpdateConstantBuffers(pContext);
}

void PostProcessor::Update(ID3D11DeviceContext* pContext)
{
	_ASSERT(pContext);
	_ASSERT(m_pPostEffectsConstsGPU);

	if (PostEffectsUpdateFlag > 0)
	{
		UpdateBuffer(pContext, PostEffectsConstsCPU, m_pPostEffectsConstsGPU);
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
	_ASSERT(pContext);

	// Resolve MSAA texture.
	pContext->ResolveSubresource(m_pResolvedBuffer, 0, m_pFloatBuffer, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);

	// 스크린 렌더링을 위한 정점 버퍼와 인텍스 버퍼를 미리 설정.
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &(pMesh->pVertexBuffer), &stride, &offset);
	pContext->IASetIndexBuffer(pMesh->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// post effecting 후 post processing.
	renderPostEffects(pContext);
	renderPostProcessing(pContext);
}

void PostProcessor::Cleanup()
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

	for (UINT64 i = 0, size = m_pBloomSRVs.size(); i < size; ++i)
	{
		RELEASE(m_pBloomSRVs[i]);
		RELEASE(m_pBloomRTVs[i]);
	}
	m_pBloomSRVs.clear();
	m_pBloomRTVs.clear();
	m_pBloomDownFilters.clear();
	m_pBloomUpFilters.clear();

	CombineFilter.Cleanup();

	SAFE_RELEASE(m_pPostEffectsConstsGPU);

	SAFE_RELEASE(m_pPostEffectsBuffer);
	SAFE_RELEASE(m_pPostEffectsRTV);
	SAFE_RELEASE(m_pPostEffectsSRV);

	if (pMesh)
	{
		delete pMesh;
		pMesh = nullptr;
	}
}

void PostProcessor::createPostBackBuffers(ID3D11Device* pDevice)
{
	_ASSERT(pDevice);

	// 후처리를 위한 back buffer.

	HRESULT hr = S_OK;
	D3D11_TEXTURE2D_DESC desc = {};
	m_pBackBuffer->GetDesc(&desc);
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	hr = pDevice->CreateTexture2D(&desc, nullptr, &m_pPostEffectsBuffer);
	BREAK_IF_FAILED(hr);

	hr = pDevice->CreateShaderResourceView(m_pPostEffectsBuffer, nullptr, &m_pPostEffectsSRV);
	BREAK_IF_FAILED(hr);

	hr = pDevice->CreateRenderTargetView(m_pPostEffectsBuffer, nullptr, &m_pPostEffectsRTV);
	BREAK_IF_FAILED(hr);
}

void PostProcessor::createImageResources(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, int width, int height, ID3D11ShaderResourceView** ppSrv, ID3D11RenderTargetView** ppRtv)
{
	_ASSERT(pDevice);
	_ASSERT(pContext);

	HRESULT hr = S_OK;

	ID3D11Texture2D* texture = nullptr;
	D3D11_TEXTURE2D_DESC txtDesc = {};
	txtDesc.Width = width;
	txtDesc.Height = height;
	txtDesc.MipLevels = txtDesc.ArraySize = 1;
	txtDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // 이미지 처리용도.
	txtDesc.SampleDesc.Count = 1;
	txtDesc.Usage = D3D11_USAGE_DEFAULT;
	txtDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS;
	txtDesc.MiscFlags = 0;
	txtDesc.CPUAccessFlags = 0;

	hr = pDevice->CreateTexture2D(&txtDesc, nullptr, &texture);
	BREAK_IF_FAILED(hr);

	hr = pDevice->CreateRenderTargetView(texture, nullptr, ppRtv);
	BREAK_IF_FAILED(hr);

	hr = pDevice->CreateShaderResourceView(texture, nullptr, ppSrv);
	BREAK_IF_FAILED(hr);

	RELEASE(texture);
}

void PostProcessor::renderPostEffects(ID3D11DeviceContext* pContext)
{
	_ASSERT(pContext);

	// PostEffects (m_pGlobalConstsGPU 사용).
	setViewport(pContext);
	setPipelineState(pContext, g_PostEffectsPSO);
	setGlobalConsts(pContext, &m_pGlobalConstsGPU);

	ID3D11ShaderResourceView* ppPostEffectsSRVs[2] = { m_pResolvedSRV, m_pDepthOnlySRV };
	pContext->PSSetShaderResources(20, 2, ppPostEffectsSRVs);
	pContext->OMSetRenderTargets(1, &m_pPostEffectsRTV, nullptr);
	pContext->PSSetConstantBuffers(5, 1, &m_pPostEffectsConstsGPU);
	pContext->DrawIndexed(pMesh->IndexCount, 0, 0);

	ID3D11ShaderResourceView* ppNulls[2] = { nullptr, };
	pContext->PSSetShaderResources(20, 2, ppNulls);
}

void PostProcessor::renderPostProcessing(ID3D11DeviceContext* pContext)
{
	_ASSERT(pContext);

	// 후처리 (블룸 같은 순수 이미지 처리).
	setPipelineState(pContext, g_PostProcessingPSO);

	pContext->PSSetSamplers(0, 1, &g_pLinearClampSS);

	// 블룸이 필요한 경우에만 계산.
	ImageFilterConstData* pCombineFilterConstData = (ImageFilterConstData*)CombineFilter.GetConstantBufferPtr()->pSystemMem;
	if (pCombineFilterConstData->Strength > 0.0f)
	{
		for (UINT64 i = 0, size = m_pBloomDownFilters.size(); i < size; ++i)
		{
			renderImageFilter(pContext, m_pBloomDownFilters[i]);
		}
		for (UINT64 i = 0, size = m_pBloomUpFilters.size(); i < size; ++i)
		{
			renderImageFilter(pContext, m_pBloomUpFilters[i]);
		}
	}

	renderImageFilter(pContext, CombineFilter);

	pContext->CopyResource(m_pPrevBuffer, m_pBackBuffer); // 모션 블러 효과를 위해 렌더링 결과 보관.
}

void PostProcessor::renderImageFilter(ID3D11DeviceContext* pContext, const ImageFilter& IMAGE_FILTER)
{
	_ASSERT(pContext);

	IMAGE_FILTER.Render(pContext);
	pContext->DrawIndexed(pMesh->IndexCount, 0, 0);
}

void PostProcessor::setViewport(ID3D11DeviceContext* pContext)
{
	_ASSERT(pContext);

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

void PostProcessor::setRenderConfig(const PostProcessingBuffers& CONFIG)
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

void PostProcessor::setPipelineState(ID3D11DeviceContext* pContext, GraphicsPSO& PSO)
{
	_ASSERT(pContext);

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
	_ASSERT(pContext);
	_ASSERT(ppGlobalConstsGPU);

	// 쉐이더와 일관성 유지 cbuffer GlobalConstants : register(b0).
	pContext->VSSetConstantBuffers(0, 1, ppGlobalConstsGPU);
	pContext->PSSetConstantBuffers(0, 1, ppGlobalConstsGPU);
	pContext->GSSetConstantBuffers(0, 1, ppGlobalConstsGPU);
}
