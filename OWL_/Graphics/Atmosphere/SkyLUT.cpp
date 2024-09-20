#include "../../Common.h"
#include "../../Renderer/ConstantBuffer.h"
#include "../../Renderer/BaseRenderer.h"
#include "../../Renderer/Texture.h"
#include "SkyLUT.h"

void SkyLUT::Initialize(BaseRenderer* pRenderer)
{
	_ASSERT(pRenderer);

	m_pRenderer = pRenderer;

	createSkyLUTBuffer();
	createConstantBuffers();
}

void SkyLUT::Update()
{
	_ASSERT(m_pSkyLUTConstantBuffer);
	m_pSkyLUTConstantBuffer->Upload();
}

void SkyLUT::Generate()
{
	_ASSERT(m_pRenderer);
	_ASSERT(m_pSkyLUTConstantBuffer);
	_ASSERT(m_pAtmosphereConstantBuffer);
	_ASSERT(m_pTransmittanceLUT);
	_ASSERT(m_pMultiScatterLUT);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	//Set PSO.
	pResourceManager->SetPipelineState(GraphicsPSOType_SkyLUT);

	// Set resources.
	ID3D11Buffer* ppConstantBuffers[2] = { m_pSkyLUTConstantBuffer->pBuffer, m_pAtmosphereConstantBuffer->pBuffer };
	ID3D11ShaderResourceView* ppSRVs[2] = { m_pTransmittanceLUT->pSRV, m_pMultiScatterLUT->pSRV };
	pContext->PSSetConstantBuffers(0, 2, ppConstantBuffers);
	pContext->PSSetShaderResources(0, 2, ppSRVs);
	pContext->PSSetSamplers(0, 1, &pResourceManager->pLinearClampSS);

	// Draw.
	setViewport();
	pContext->OMSetRenderTargets(1, &m_pSkyLUT->pRTV, nullptr);
	pContext->Draw(6, 0);

	// Reset resources.
	ID3D11Buffer* ppNullConstantBuffers[2] = { nullptr, };
	ID3D11ShaderResourceView* ppNullSRVs[2] = { nullptr, };
	ID3D11SamplerState* pNullSampler = nullptr;
	ID3D11RenderTargetView* pNullRTV = nullptr;
	pContext->PSSetConstantBuffers(0, 2, ppNullConstantBuffers);
	pContext->PSSetShaderResources(0, 2, ppNullSRVs);
	pContext->PSSetSamplers(0, 1, &pNullSampler);
	pContext->OMSetRenderTargets(1, &pNullRTV, nullptr);
}

void SkyLUT::Resize()
{
	if (m_pSkyLUT)
	{
		delete m_pSkyLUT;
		m_pSkyLUT = nullptr;
	}
	
	createSkyLUTBuffer();
}

void SkyLUT::Cleanup()
{
	if (m_pSkyLUT)
	{
		delete m_pSkyLUT;
		m_pSkyLUT = nullptr;
	}
	if (m_pSkyLUTConstantBuffer)
	{
		delete m_pSkyLUTConstantBuffer;
		m_pSkyLUTConstantBuffer = nullptr;
	}

	pSkyData = nullptr;
	m_pRenderer = nullptr;
	m_pAtmosphereConstantBuffer = nullptr;
	m_pTransmittanceLUT = nullptr;
	m_pMultiScatterLUT = nullptr;
}

void SkyLUT::SetCamera(const Vector3* const pAtmosEyePos)
{
	_ASSERT(m_pSkyLUTConstantBuffer);
	_ASSERT(pAtmosEyePos);

	SkyLUTConstants* pSkyLUTData = (SkyLUTConstants*)m_pSkyLUTConstantBuffer->pSystemMem;
	if (!pSkyLUTData)
	{
		__debugbreak();
	}

	pSkyLUTData->AtmosEyePos = *pAtmosEyePos;
}

void SkyLUT::SetSun(const Vector3* const pDirection, const Vector3* const pIntensity)
{
	_ASSERT(m_pSkyLUTConstantBuffer);
	_ASSERT(pDirection);
	_ASSERT(pIntensity);

	SkyLUTConstants* pSkyLUTData = (SkyLUTConstants*)m_pSkyLUTConstantBuffer->pSystemMem;
	if (!pSkyLUTData)
	{
		__debugbreak();
	}

	pSkyLUTData->SunDirection = *pDirection;
	pSkyLUTData->SunIntensity = *pIntensity;
}

void SkyLUT::createSkyLUTBuffer()
{
	_ASSERT(m_pRenderer);
	_ASSERT(!m_pSkyLUT);

	ID3D11Device* pDevice = m_pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	D3D11_TEXTURE2D_DESC resourceDesc = {};
	resourceDesc.Width = (UINT)Resolution.x;
	resourceDesc.Height = (UINT)Resolution.y;
	resourceDesc.MipLevels = 1;
	resourceDesc.ArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Usage = D3D11_USAGE_DEFAULT;
	resourceDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	resourceDesc.CPUAccessFlags = 0;
	resourceDesc.MiscFlags = 0;

	m_pSkyLUT = New Texture;
	m_pSkyLUT->Initialize(pDevice, pContext, resourceDesc, nullptr, true);
}

void SkyLUT::createConstantBuffers()
{
	_ASSERT(m_pRenderer);
	_ASSERT(!m_pSkyLUTConstantBuffer);

	ID3D11Device* pDevice = m_pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	m_pSkyLUTConstantBuffer = New ConstantBuffer;
	m_pSkyLUTConstantBuffer->Initialize(pDevice, pContext, sizeof(SkyLUTConstants), nullptr);

	pSkyData = (SkyLUTConstants*)m_pSkyLUTConstantBuffer->pSystemMem;
	pSkyData->LowResMarchStepCount = 40;
}

void SkyLUT::setViewport()
{
	_ASSERT(m_pRenderer);

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = Resolution.x;
	viewport.Height = Resolution.y;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	m_pRenderer->SetViewport(&viewport, 1);
}
