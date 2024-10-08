#include "../Common.h"
#include "BaseRenderer.h"
#include "ConstantBuffer.h"
#include "../Graphics/ConstantDataType.h"
#include "../Geometry/GeometryGenerator.h"
#include "../Graphics/GraphicsUtils.h"
#include "../Geometry/Mesh.h"
#include "../Geometry/MeshInfo.h"
#include "../Geometry/Model.h"
#include "PostProcessor.h"

void PostProcessor::Initialize(BaseRenderer* pRenderer, const PostProcessingBuffers& CONFIG, const int WIDTH, const int HEIGHT, const int BLOOMLEVELS)
{
	_ASSERT(pRenderer);

	HRESULT hr = S_OK;

	m_pRenderer = pRenderer;
	m_ScreenWidth = WIDTH;
	m_ScreenHeight = HEIGHT;

	ResourceManager* pResourceManager = pRenderer->GetResourceManager();
	ID3D11Device* pDevice = pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = pRenderer->GetDeviceContext();

	// 후처리 효과용 버퍼 생성.
	setRenderConfig(CONFIG);
	createPostBackBuffers();

	// 후처리 효과용 constBuffer.
	PostEffectsConstants initData;
	m_pPostEffectsConstantBuffer = new ConstantBuffer;
	m_pPostEffectsConstantBuffer->Initialize(pDevice, pContext, sizeof(PostEffectsConstants), &initData);

	// Bloom Down/Up.
	m_pBloomSRVs.resize(BLOOMLEVELS);
	m_pBloomRTVs.resize(BLOOMLEVELS);
	for (int i = 0; i < BLOOMLEVELS; ++i)
	{
		int div = (int)pow(2, i);
		createImageResources(WIDTH / div, HEIGHT / div, &m_pBloomSRVs[i], &m_pBloomRTVs[i]);
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
		m_pBloomDownFilters[i].Initialize(pDevice, pContext, pResourceManager->pBloomDownPS, WIDTH / div, HEIGHT / div);
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
		m_pBloomUpFilters[i].Initialize(pDevice, pContext, pResourceManager->pBloomUpPS, WIDTH / div, HEIGHT / div);
		m_pBloomUpFilters[i].SetShaderResources({ m_pBloomSRVs[level + 1] });
		m_pBloomUpFilters[i].SetRenderTargets({ m_pBloomRTVs[level] });
	}

	// combine + tone mapping.
	CombineFilter.Initialize(pDevice, pContext, pResourceManager->pCombinePS, WIDTH, HEIGHT);
	CombineFilter.SetShaderResources({ m_pPostEffectsSRV, m_pBloomSRVs[0], m_pPrevSRV }); // resource[1]은 모션 블러를 위한 이전 프레임 결과.
	CombineFilter.SetRenderTargets({ m_pBackBufferRTV });
	
	ImageFilterConstData* pCombineFilterConstData = (ImageFilterConstData*)CombineFilter.GetConstantBufferPtr()->pSystemMem;
	pCombineFilterConstData->Strength = 0.0f;	// bloom strength.
	pCombineFilterConstData->Option1 = 1.0f;	// exposure.
	pCombineFilterConstData->Option2 = 2.2f;	// gamma.

	// 주의: float render target에서는 gamma correction 하지 않음. (gamma = 1.0)
	m_pPostEffectsConstantBuffer->Upload();
	CombineFilter.UpdateConstantBuffers(pContext);
}

void PostProcessor::Update()
{
	_ASSERT(m_pRenderer);
	_ASSERT(m_pPostEffectsConstantBuffer);

	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	if (PostEffectsUpdateFlag > 0)
	{
		m_pPostEffectsConstantBuffer->Upload();
		PostEffectsUpdateFlag = 0;
	}

	if (CombineUpdateFlag > 0)
	{
		CombineFilter.UpdateConstantBuffers(pContext);
		CombineUpdateFlag = 0;
	}
}

void PostProcessor::Render()
{
	_ASSERT(m_pRenderer);

	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	// Resolve MSAA texture.
	pContext->ResolveSubresource(m_pResolvedBuffer, 0, m_pFloatBuffer, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);

	// post effecting 후 post processing.
	renderPostEffects();
	renderPostProcessing();
}

void PostProcessor::Cleanup()
{
	m_pRenderer = nullptr;
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

	//SAFE_RELEASE(m_pPostEffectsConstsGPU);
	if (m_pPostEffectsConstantBuffer)
	{
		delete m_pPostEffectsConstantBuffer;
		m_pPostEffectsConstantBuffer = nullptr;
	}

	SAFE_RELEASE(m_pPostEffectsBuffer);
	SAFE_RELEASE(m_pPostEffectsRTV);
	SAFE_RELEASE(m_pPostEffectsSRV);
}

void PostProcessor::createPostBackBuffers()
{
	_ASSERT(m_pRenderer);

	// 후처리를 위한 back buffer.

	ID3D11Device* pDevice = m_pRenderer->GetDevice();

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

void PostProcessor::createImageResources(int width, int height, ID3D11ShaderResourceView** ppSrv, ID3D11RenderTargetView** ppRtv)
{
	_ASSERT(m_pRenderer);

	HRESULT hr = S_OK;
	ID3D11Device* pDevice = m_pRenderer->GetDevice();

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

void PostProcessor::renderPostEffects()
{
	_ASSERT(m_pRenderer);
	_ASSERT(m_pPostEffectsConstantBuffer);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext(); 

	// PostEffects (m_pGlobalConstsGPU 사용).
	setViewport();
	pResourceManager->SetPipelineState(GraphicsPSOType_PostEffects);
	setGlobalConsts(&m_pGlobalConstsGPU);

	ID3D11ShaderResourceView* ppPostEffectsSRVs[2] = { m_pResolvedSRV, m_pDepthOnlySRV };
	pContext->PSSetShaderResources(20, 2, ppPostEffectsSRVs);
	pContext->OMSetRenderTargets(1, &m_pPostEffectsRTV, nullptr);
	pContext->PSSetConstantBuffers(5, 1, &m_pPostEffectsConstantBuffer->pBuffer);
	pContext->Draw(6, 0);

	ID3D11ShaderResourceView* ppNulls[2] = { nullptr, };
	pContext->PSSetShaderResources(20, 2, ppNulls);
}

void PostProcessor::renderPostProcessing()
{
	_ASSERT(m_pRenderer);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	// 후처리 (블룸 같은 순수 이미지 처리).
	pResourceManager->SetPipelineState(GraphicsPSOType_PostProcessing);

	pContext->PSSetSamplers(0, 1, &pResourceManager->pLinearClampSS);

	// 블룸이 필요한 경우에만 계산.
	ImageFilterConstData* pCombineFilterConstData = (ImageFilterConstData*)CombineFilter.GetConstantBufferPtr()->pSystemMem;
	if (pCombineFilterConstData->Strength > 0.0f)
	{
		for (UINT64 i = 0, size = m_pBloomDownFilters.size(); i < size; ++i)
		{
			renderImageFilter(m_pBloomDownFilters[i]);
		}
		for (UINT64 i = 0, size = m_pBloomUpFilters.size(); i < size; ++i)
		{
			renderImageFilter(m_pBloomUpFilters[i]);
		}
	}

	renderImageFilter(CombineFilter);

	pContext->CopyResource(m_pPrevBuffer, m_pBackBuffer); // 모션 블러 효과를 위해 렌더링 결과 보관.
}

void PostProcessor::renderImageFilter(const ImageFilter& IMAGE_FILTER)
{
	_ASSERT(m_pRenderer);

	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();
	IMAGE_FILTER.Render(pContext);
	pContext->Draw(6, 0);
}

void PostProcessor::setViewport()
{
	_ASSERT(m_pRenderer);

	// Set the viewport
	m_Viewport.TopLeftX = 0;
	m_Viewport.TopLeftY = 0;
	m_Viewport.Width = (float)m_ScreenWidth;
	m_Viewport.Height = (float)m_ScreenHeight;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	m_pRenderer->SetViewport(&m_Viewport, 1);
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

void PostProcessor::setGlobalConsts(ID3D11Buffer** ppGlobalConstsGPU)
{
	_ASSERT(m_pRenderer);
	
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	// 쉐이더와 일관성 유지 cbuffer GlobalConstants : register(b0).
	pContext->VSSetConstantBuffers(0, 1, ppGlobalConstsGPU);
	pContext->PSSetConstantBuffers(0, 1, ppGlobalConstsGPU);
	pContext->GSSetConstantBuffers(0, 1, ppGlobalConstsGPU);
}
