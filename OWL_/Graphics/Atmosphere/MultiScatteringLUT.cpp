#include <random>
#include "../../Common.h"
#include "../Renderer/BaseRenderer.h"
#include "../Renderer/ConstantBuffer.h"
#include "../Renderer/StructuredBuffer.h"
#include "../Renderer/Texture.h"
#include "MultiScatteringLUT.h"

void GetPoissonDiskSamples(std::vector<Vector2>& samples, const int COUNT)
{
	std::default_random_engine rng{ std::random_device()() };
	std::uniform_real_distribution<float> dis(0, 1);

	std::vector<Vector2> rawPoints;
	rawPoints.reserve(COUNT * 10);
	for (int i = 0, endI = COUNT * 10; i < endI; ++i)
	{
		const float U = dis(rng);
		const float V = dis(rng);
		rawPoints.push_back(Vector2(U, V));
	}

	std::vector<Vector2> outputPoints(COUNT);

	cy::WeightedSampleElimination<Vector2, float, 2> wse;
	wse.SetTiling(true);
	wse.Eliminate(rawPoints.data(), rawPoints.size(),
				  outputPoints.data(), outputPoints.size());

	samples.reserve(outputPoints.size());
	for (auto& p : outputPoints)
		samples.push_back(p);
}


void MultiScatteringLUT::Initialize(BaseRenderer* pRenderer)
{
	_ASSERT(pRenderer);

	m_pRenderer = pRenderer;

	createConstantBuffer();
	createMultiScatteringLUTBuffer();
	createRawDiskSamples();
}

void MultiScatteringLUT::Update(const Vector3* const pTerrainAlbedo)
{
	_ASSERT(m_pMultiScatteringConstantBuffer);
	_ASSERT(pTerrainAlbedo);

	MultiScatteringConstants* pMultiScatteringConstData = (MultiScatteringConstants*)m_pMultiScatteringConstantBuffer->pSystemMem;
	if (!pMultiScatteringConstData)
	{
		__debugbreak();
	}

	pMultiScatteringConstData->TerainAlbedo = *pTerrainAlbedo;

	m_pMultiScatteringConstantBuffer->Upload();
}

void MultiScatteringLUT::Generate(Texture* pTransmittanceLUT)
{
	_ASSERT(m_pRenderer);
	_ASSERT(m_pMultiScatteringConstantBuffer);
	_ASSERT(m_pAtmosphereConstantBuffer);
	_ASSERT(m_pMultiScatteringLUT);
	_ASSERT(pTransmittanceLUT);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	// Set PSO.
	pResourceManager->SetPipelineState(ComputePSOType_MultiScatterLUT);


	// Set resources.
	ID3D11Buffer* ppConstantBuffers[2] = { m_pMultiScatteringConstantBuffer->pBuffer, m_pAtmosphereConstantBuffer->pBuffer };
	ID3D11ShaderResourceView* ppSRVs[2] = { pTransmittanceLUT->pSRV, m_pRawSamples->pSRV };
	pContext->CSSetConstantBuffers(0, 2, ppConstantBuffers);
	pContext->CSSetShaderResources(0, 2, ppSRVs);
	pContext->CSSetSamplers(0, 1, &pResourceManager->pLinearClampSS);
	pContext->CSSetUnorderedAccessViews(0, 1, &m_pMultiScatteringLUT->pUAV, nullptr);


	// Dispatch.
	const int THREAD_GROUP_SIZE_X = 16;
	const int THREAD_GROUP_SIZE_Y = 16;
	const int THREAD_GROUP_COUNT_X = (Resolution.x + THREAD_GROUP_SIZE_X - 1) / THREAD_GROUP_SIZE_X;
	const int THREAD_GROUP_COUNT_Y = (Resolution.y + THREAD_GROUP_SIZE_Y - 1) / THREAD_GROUP_SIZE_Y;
	pContext->Dispatch(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1);


	// Reset resources.
	ID3D11Buffer* ppNullConstantBuffers[2] = { nullptr, };
	ID3D11ShaderResourceView* ppNullSRVs[2] = { nullptr, };
	ID3D11SamplerState* pNullSampler = nullptr;
	ID3D11UnorderedAccessView* pNullUAV = nullptr;
	pContext->CSSetConstantBuffers(0, 2, ppNullConstantBuffers);
	pContext->CSSetShaderResources(0, 2, ppNullSRVs);
	pContext->CSSetSamplers(0, 1, &pNullSampler);
	pContext->CSSetUnorderedAccessViews(0, 1, &pNullUAV, nullptr);
}

void MultiScatteringLUT::Cleanup()
{
	if (m_pMultiScatteringConstantBuffer)
	{
		delete m_pMultiScatteringConstantBuffer;
		m_pMultiScatteringConstantBuffer = nullptr;
	}
	if (m_pMultiScatteringLUT)
	{
		delete m_pMultiScatteringLUT;
		m_pMultiScatteringLUT = nullptr;
	}
	if (m_pRawSamples)
	{
		delete m_pRawSamples;
		m_pRawSamples = nullptr;
	}

	m_pRenderer = nullptr;
	m_pAtmosphereConstantBuffer = nullptr;
}

void MultiScatteringLUT::createConstantBuffer()
{
	_ASSERT(m_pRenderer);
	_ASSERT(!m_pMultiScatteringConstantBuffer);

	ID3D11Device* pDevice = m_pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	m_pMultiScatteringConstantBuffer = New ConstantBuffer;
	m_pMultiScatteringConstantBuffer->Initialize(pDevice, pContext, sizeof(MultiScatteringConstants), nullptr);

	MultiScatteringConstants* pMultiScatterConstData = (MultiScatteringConstants*)m_pMultiScatteringConstantBuffer->pSystemMem;
	if (!pMultiScatterConstData)
	{
		__debugbreak();
	}
	pMultiScatterConstData->TerainAlbedo = Vector3(0.3f);
	pMultiScatterConstData->DirSampleCount = DIR_SAMPLE_COUNT;
	pMultiScatterConstData->SunIntensity = Vector3(1.0f);
	pMultiScatterConstData->RayMarchStepCount = RAY_MARCH_STEP_COUNT;
}

void MultiScatteringLUT::createMultiScatteringLUTBuffer()
{
	_ASSERT(m_pRenderer);
	_ASSERT(!m_pMultiScatteringLUT);

	ID3D11Device* pDevice = m_pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = (UINT)Resolution.x;
	textureDesc.Height = (UINT)Resolution.y;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.SampleDesc = { 1, 0 };
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	m_pMultiScatteringLUT = New Texture;
	m_pMultiScatteringLUT->Initialize(pDevice, pContext, textureDesc, nullptr, false);


	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	m_pMultiScatteringLUT->CreateSRV(srvDesc);

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	m_pMultiScatteringLUT->CreateUAV(uavDesc);
}

void MultiScatteringLUT::createRawDiskSamples()
{
	_ASSERT(m_pRenderer);
	_ASSERT(!m_pRawSamples);

	ID3D11Device* pDevice = m_pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	std::vector<Vector2> rawSamples;
	GetPoissonDiskSamples(rawSamples, DIR_SAMPLE_COUNT);

	m_pRawSamples = New StructuredBuffer;
	m_pRawSamples->Initialize(pDevice, pContext, sizeof(Vector2), (UINT)rawSamples.size(), rawSamples.data());
}
