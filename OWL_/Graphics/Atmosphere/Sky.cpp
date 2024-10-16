#include "../Common.h"
#include "../Renderer/BaseRenderer.h"
#include "../Graphics/Camera.h"
#include "../Renderer/ConstantBuffer.h"
#include "../Renderer/Texture.h"
#include "../Renderer/ResourceManager.h"
#include "Sky.h"

void Sky::Initialize(BaseRenderer* pRenderer)
{
	_ASSERT(pRenderer);

	m_pRenderer = pRenderer;

	createConstantBuffer();
}

void Sky::Update()
{
	_ASSERT(m_pSkyConstantBuffer);
	m_pSkyConstantBuffer->Upload();
}

void Sky::Render(Texture* pSkyLUT)
{
	_ASSERT(m_pRenderer);
	_ASSERT(pSkyLUT);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	// Set PSO.
	pResourceManager->SetPipelineState(GraphicsPSOType_Sky);

	// Set resources.
	pContext->PSSetConstantBuffers(0, 1, &m_pSkyConstantBuffer->pBuffer);
	pContext->PSSetShaderResources(0, 1, &pSkyLUT->pSRV);
	pContext->PSSetSamplers(0, 1, &pResourceManager->pSkyLUTSS);

	// Draw.
	pContext->Draw(6, 0);

	// Reset resources.
	ID3D11Buffer* pNullConstantBuffer = nullptr;
	ID3D11ShaderResourceView* pNullSRV = nullptr;
	ID3D11SamplerState* pNullSampler = nullptr;
	pContext->PSSetConstantBuffers(0, 1, &pNullConstantBuffer);
	pContext->PSSetShaderResources(0, 1, &pNullSRV);
	pContext->PSSetSamplers(0, 1, &pNullSampler);
}

void Sky::Cleanup()
{
	if (m_pSkyConstantBuffer)
	{
		delete m_pSkyConstantBuffer;
		m_pSkyConstantBuffer = nullptr;
	}

	m_pRenderer = nullptr;
}

void Sky::SetCamera(const FrustumDirection* pFrustumDirs)
{
	_ASSERT(pFrustumDirs);
	_ASSERT(m_pSkyConstantBuffer);

	SkyConstants* pSkyConstantData = (SkyConstants*)m_pSkyConstantBuffer->pSystemMem;
	if (!pSkyConstantData)
	{
		__debugbreak();
	}

	pSkyConstantData->FrustumA = pFrustumDirs->FrustumA;
	pSkyConstantData->FrustumB = pFrustumDirs->FrustumB;
	pSkyConstantData->FrustumC = pFrustumDirs->FrustumC;
	pSkyConstantData->FrustumD = pFrustumDirs->FrustumD;
}

void Sky::createConstantBuffer()
{
	_ASSERT(m_pRenderer);
	_ASSERT(!m_pSkyConstantBuffer);

	ID3D11Device* pDevice = m_pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	m_pSkyConstantBuffer = new ConstantBuffer;
	m_pSkyConstantBuffer->Initialize(pDevice, pContext, sizeof(SkyConstants), nullptr);
}
