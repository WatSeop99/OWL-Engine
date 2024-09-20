#include "../../Common.h"
#include "../../Renderer/BaseRenderer.h"
#include "../Camera.h"

#include "../../Renderer/Texture.h"
#include "AerialLUT.h"

void AerialLUT::Initialize(BaseRenderer* pRenderer)
{
	_ASSERT(pRenderer);

	m_pRenderer = pRenderer;

	createAerialLUTBuffer();
	createConstantBuffer();
}

void AerialLUT::Update()
{
	_ASSERT(m_pAerialConstantBuffer);
	m_pAerialConstantBuffer->Upload();
}

void AerialLUT::Generate(Texture* pShadowMap)
{
	_ASSERT(m_pRenderer);
	_ASSERT(m_pAerialConstantBuffer);
	_ASSERT(m_pAtmosphereConstantBuffer);
	_ASSERT(m_pMultiScatterLUT);
	_ASSERT(m_pTransmittanceLUT);
	_ASSERT(pShadowMap);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	// Set PSO.
	pResourceManager->SetPipelineState(ComputePSOType_AerialLUT);

	// Set resources.
	ID3D11Buffer* ppConstantBuffers[2] = { m_pAerialConstantBuffer->pBuffer, m_pAtmosphereConstantBuffer->pBuffer };
	ID3D11ShaderResourceView* ppSRVs[3] = { m_pMultiScatterLUT->pSRV, m_pTransmittanceLUT->pSRV, pShadowMap->pSRV };
	ID3D11SamplerState* ppSamplers[2] = { pResourceManager->pLinearClampSS, pResourceManager->pPointClampSS };
	pContext->CSSetConstantBuffers(0, 2, ppConstantBuffers);
	pContext->CSSetShaderResources(0, 3, ppSRVs);
	pContext->CSSetSamplers(0, 2, ppSamplers);
	pContext->CSSetUnorderedAccessViews(0, 1, &m_pAerialLUT->pUAV, nullptr);

	// Dispatch.
	const int THREAD_GROUP_SIZE_X = 16;
	const int THREAD_GROUP_SIZE_Y = 16;
	const int THREAD_GROUP_COUNT_X = (Resolution.x + THREAD_GROUP_SIZE_X - 1) / THREAD_GROUP_SIZE_X;
	const int THREAD_GROUP_COUNT_Y = (Resolution.y + THREAD_GROUP_SIZE_Y - 1) / THREAD_GROUP_SIZE_Y;
	pContext->Dispatch(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1);


	// Reset resources.
	ID3D11Buffer* ppNullConstantBuffers[2] = { nullptr, };
	ID3D11ShaderResourceView* ppNullSRVs[3] = { nullptr, };
	ID3D11SamplerState* ppNullSamplers[2] = { nullptr, };
	ID3D11UnorderedAccessView* pNullUAV = nullptr;
	pContext->CSSetConstantBuffers(0, 2, ppNullConstantBuffers);
	pContext->CSSetShaderResources(0, 3, ppNullSRVs);
	pContext->CSSetSamplers(0, 2, ppNullSamplers);
	pContext->CSSetUnorderedAccessViews(0, 1, &pNullUAV, nullptr);
}

void AerialLUT::Resize()
{
	if (m_pAerialLUT)
	{
		delete m_pAerialLUT;
		m_pAerialLUT = nullptr;
	}
	createAerialLUTBuffer();
}

void AerialLUT::Cleanup()
{
	if (m_pAerialConstantBuffer)
	{
		delete m_pAerialConstantBuffer;
		m_pAerialConstantBuffer = nullptr;
	}
	if (m_pAerialLUT)
	{
		delete m_pAerialLUT;
		m_pAerialLUT = nullptr;
	}

	pAerialData = nullptr;
	Resolution = Vector3(200.0f, 150.0f, 32.0f);
	m_pRenderer = nullptr;
	m_pAtmosphereConstantBuffer = nullptr;
	m_pTransmittanceLUT = nullptr;
	m_pMultiScatterLUT = nullptr;
}

void AerialLUT::SetCamera(const Vector3* const pEyePos, const float ATMOS_EYE_HEIGHT, const FrustumDirection* const pFrustumDirs)
{
	_ASSERT(m_pAerialConstantBuffer);
	_ASSERT(pEyePos);
	_ASSERT(pFrustumDirs);

	AerialLUTConstants* pAerialConstData = (AerialLUTConstants*)m_pAerialConstantBuffer->pSystemMem;
	if (!pAerialConstData)
	{
		__debugbreak();
	}

	pAerialConstData->ShadowEyePosition = *pEyePos;
	pAerialConstData->EyePositionY = ATMOS_EYE_HEIGHT;
	pAerialConstData->FrustumA = pFrustumDirs->FrustumA;
	pAerialConstData->FrustumB = pFrustumDirs->FrustumB;
	pAerialConstData->FrustumC = pFrustumDirs->FrustumC;
	pAerialConstData->FrustumD = pFrustumDirs->FrustumD;
}

void AerialLUT::SetSun(const Vector3* const pDirection)
{
	_ASSERT(m_pAerialConstantBuffer);
	_ASSERT(pDirection);

	AerialLUTConstants* pAerialConstData = (AerialLUTConstants*)m_pAerialConstantBuffer->pSystemMem;
	if (!pAerialConstData)
	{
		__debugbreak();
	}

	pAerialConstData->SunDirection = *pDirection;
	pAerialConstData->SunTheta = (float)asin(-pAerialConstData->SunDirection.y);
}

void AerialLUT::createAerialLUTBuffer()
{
	_ASSERT(m_pRenderer);
	_ASSERT(!m_pAerialLUT);

	ID3D11Device* pDevice = m_pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	D3D11_TEXTURE3D_DESC resourceDesc = {};
	resourceDesc.Width = (UINT)Resolution.x;
	resourceDesc.Height = (UINT)Resolution.y;
	resourceDesc.Depth = (UINT)Resolution.z;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	resourceDesc.Usage = D3D11_USAGE_DEFAULT;
	resourceDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	resourceDesc.CPUAccessFlags = 0;
	resourceDesc.MiscFlags = 0;

	m_pAerialLUT = New Texture;
	m_pAerialLUT->Initialize(pDevice, pContext, resourceDesc, nullptr, false);

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	uavDesc.Texture3D.FirstWSlice = 0;
	uavDesc.Texture3D.WSize = (UINT)Resolution.z;
	uavDesc.Texture3D.MipSlice = 0;
	m_pAerialLUT->CreateUAV(uavDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc ={};
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Texture3D.MipLevels = 1;
	srvDesc.Texture3D.MostDetailedMip = 0;
	m_pAerialLUT->CreateSRV(srvDesc);
}

void AerialLUT::createConstantBuffer()
{
	_ASSERT(m_pRenderer);
	_ASSERT(!m_pAerialConstantBuffer);

	ID3D11Device* pDevice = m_pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	m_pAerialConstantBuffer = New ConstantBuffer;
	m_pAerialConstantBuffer->Initialize(pDevice, pContext, sizeof(AerialLUTConstants), nullptr);
	pAerialData = (AerialLUTConstants*)m_pAerialConstantBuffer->pSystemMem;

	pAerialData->MaxDistance = 2000.0f;
	pAerialData->PerSliceStepCount = 1;
	pAerialData->ShadowViewProj = Matrix();
	pAerialData->WorldScale = 200.0f;
}
