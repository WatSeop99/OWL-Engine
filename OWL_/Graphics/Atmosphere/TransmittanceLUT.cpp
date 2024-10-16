#include "../../Common.h"
#include "../Renderer/BaseRenderer.h"
#include "../Renderer/ConstantBuffer.h"
#include "../Renderer/Texture.h"
#include "../Renderer/ResourceManager.h"
#include "TransmittanceLUT.h"

void TransmittanceLUT::Initialize(BaseRenderer* pRenderer)
{
	_ASSERT(pRenderer);

	m_pRenderer = pRenderer;

	createTransmittanceLUTBuffer();
}

void TransmittanceLUT::Generate()
{
	_ASSERT(m_pRenderer);
	_ASSERT(m_pAtmosphereConstantbuffer);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	// Set pso.
	pResourceManager->SetPipelineState(ComputePSOType_TransmittanceLUT);


	// Set resources.
	ID3D11Buffer* ppConstantBuffers[2] = { nullptr, m_pAtmosphereConstantbuffer->pBuffer };
	pContext->CSSetConstantBuffers(0, 2, ppConstantBuffers);
	pContext->CSSetUnorderedAccessViews(0, 1, &m_pTransmittanceLUT->pUAV, nullptr);


	// Dispatch.
	const int THREAD_GROUP_SIZE_X = 16;
	const int THREAD_GROUP_SIZE_Y = 16;
	const int THREAD_GROUP_COUNT_X = (Resolution.x + THREAD_GROUP_SIZE_X - 1) / THREAD_GROUP_SIZE_X;
	const int THREAD_GROUP_COUNT_Y = (Resolution.y + THREAD_GROUP_SIZE_Y - 1) / THREAD_GROUP_SIZE_Y;
	pContext->Dispatch(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1);


	// Reset resources.
	ID3D11Buffer* ppNullConstantBuffers[2] = { nullptr, };
	ID3D11UnorderedAccessView* pNullUAV = nullptr;
	pContext->CSSetConstantBuffers(0, 2, ppNullConstantBuffers);
	pContext->CSSetUnorderedAccessViews(0, 1, &pNullUAV, nullptr);
}

void TransmittanceLUT::ResetLUT()
{
	if (m_pTransmittanceLUT)
	{
		delete m_pTransmittanceLUT;
		m_pTransmittanceLUT = nullptr;
	}
	createTransmittanceLUTBuffer();
}

void TransmittanceLUT::Cleanup()
{
	if (m_pTransmittanceLUT)
	{
		delete m_pTransmittanceLUT;
		m_pTransmittanceLUT = nullptr;
	}

	m_pRenderer = nullptr;
}

void TransmittanceLUT::createTransmittanceLUTBuffer()
{
	_ASSERT(m_pRenderer);
	_ASSERT(!m_pTransmittanceLUT);

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

	m_pTransmittanceLUT = new Texture;
	m_pTransmittanceLUT->Initialize(pDevice, pContext, textureDesc, nullptr, false);


	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	m_pTransmittanceLUT->CreateSRV(srvDesc);

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	m_pTransmittanceLUT->CreateUAV(uavDesc);
}
