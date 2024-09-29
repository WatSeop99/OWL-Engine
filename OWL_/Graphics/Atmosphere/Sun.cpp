#include "../../Common.h"
#include "../Renderer/BaseRenderer.h"
#include "../Renderer/ConstantBuffer.h"
#include "../Renderer/Texture.h"
#include "Sun.h"

void Sun::Initialize(BaseRenderer* pRenderer)
{
	_ASSERT(pRenderer);

	m_pRenderer = pRenderer;

	createSunMesh(SunDiskSegments);
	createConstantBuffers();
}

void Sun::Update()
{
	_ASSERT(m_pSunVSConstantBuffer);
	_ASSERT(m_pSunPSConstantBuffer);

	// Update sun data.
	const float SUN_RAD_X = DegreeToRadian(SunAngle.x);
	const float SUN_RAD_Y = DegreeToRadian(-SunAngle.y);
	SunDirection = Vector3(cos(SUN_RAD_X) * cos(SUN_RAD_Y), sin(SUN_RAD_Y), sin(SUN_RAD_X) * cos(SUN_RAD_Y));
	SunDirection.Normalize();

	SunRadiance = SunIntensity * SunColor;
	SunViewProjection = Matrix::CreateLookAt(-SunDirection * 20.0f, Vector3::Zero, Vector3::UnitY) * Matrix::CreateOrthographic(20.0f, 20.0f, 0.1f, 80.0f);


	// Update VS constant buffer.
	SunVSConstants* pSunVSConstData = (SunVSConstants*)m_pSunVSConstantBuffer->pSystemMem;
	if (!pSunVSConstData)
	{
		__debugbreak();
	}

	Vector3 axisZ(SunDirection);
	axisZ.Normalize();

	Vector3 axisY;
	if (abs(abs(axisZ.Dot(Vector3::UnitX)) - 1.0f) < 0.1f)
	{
		axisY = axisZ.Cross(Vector3::UnitY);
	}
	else
	{
		axisY = axisZ.Cross(Vector3::UnitX);
	}
	
	Vector3 axisX = axisY.Cross(axisZ);

	const float SUN_THETA = asin(-SunDirection.y);
	SunWorld = Matrix::CreateScale(SunRadius) *
		Matrix(axisX, axisY, axisZ) *
		Matrix::CreateTranslation(m_CameraEye) *
		Matrix::CreateTranslation(-SunDirection);
	

	pSunVSConstData->WorldViewProjection = (SunWorld * m_CameraViewProjection).Transpose();
	pSunVSConstData->SunTheta = SUN_THETA;
	pSunVSConstData->EyeHeight = m_WorldScale * m_CameraEye.y;


	// Update PS constant buffer.
	SunPSConstants* pSunPSConstData = (SunPSConstants*)m_pSunPSConstantBuffer->pSystemMem;
	if (!pSunPSConstData)
	{
		__debugbreak();
	}
	pSunPSConstData->Radiance = SunRadiance;


	m_pSunVSConstantBuffer->Upload();
	m_pSunPSConstantBuffer->Upload();
}

void Sun::Render()
{
	_ASSERT(m_pRenderer);
	_ASSERT(m_pSunVSConstantBuffer);
	_ASSERT(m_pSunPSConstantBuffer);
	_ASSERT(m_pAtmosphereConstantBuffer);
	_ASSERT(m_pTransmittanceLUT);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	// Set PSO.
	pResourceManager->SetPipelineState(GraphicsPSOType_Sun);


	// Set resources.
	ID3D11Buffer* ppVSConstantBufffers[2] = { m_pSunVSConstantBuffer->pBuffer, m_pAtmosphereConstantBuffer->pBuffer };
	ID3D11Buffer* ppPSConstantBuffers[2] = { m_pSunPSConstantBuffer->pBuffer, nullptr };
	pContext->VSSetConstantBuffers(0, 2, ppVSConstantBufffers);
	pContext->VSSetShaderResources(0, 1, &m_pTransmittanceLUT->pSRV);
	pContext->VSSetSamplers(0, 1, &pResourceManager->pLinearClampSS);
	pContext->PSSetConstantBuffers(0, 2, ppPSConstantBuffers);


	// Draw.
	UINT stride = sizeof(Vector2);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &m_pSunDiskBuffer, &stride, &offset);
	pContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	pContext->Draw((UINT)m_SunDiskVertices.size(), 0);


	// Reset resources.
	ID3D11Buffer* ppNullConstantBufffers[2] = { nullptr, };
	ID3D11ShaderResourceView* pNullSRV = nullptr;
	ID3D11SamplerState* pNullSampler = nullptr;
	ID3D11Buffer* pNullVertexBuffer = nullptr;
	offset = 0;
	pContext->VSSetConstantBuffers(0, 2, ppNullConstantBufffers);
	pContext->VSSetShaderResources(0, 1, &pNullSRV);
	pContext->VSSetSamplers(0, 1, &pNullSampler);
	pContext->PSSetConstantBuffers(0, 2, ppNullConstantBufffers);
	pContext->IASetVertexBuffers(0, 1, &pNullVertexBuffer, &offset, &offset);
}

void Sun::ResetSunDisk()
{
	SAFE_RELEASE(m_pSunDiskBuffer);
	createSunMesh(SunDiskSegments);
}

void Sun::Cleanup()
{
	m_SunDiskVertices.clear();
	SAFE_RELEASE(m_pSunDiskBuffer);
	if (m_pSunVSConstantBuffer)
	{
		delete m_pSunVSConstantBuffer;
		m_pSunVSConstantBuffer = nullptr;
	}
	if (m_pSunPSConstantBuffer)
	{
		delete m_pSunPSConstantBuffer;
		m_pSunPSConstantBuffer = nullptr;
	}

	m_pRenderer = nullptr;
	m_pAtmosphereConstantBuffer = nullptr;
	m_pTransmittanceLUT = nullptr;
}

void Sun::SetCamera(const Vector3* const pEye, const Matrix* const pViewProj)
{
	_ASSERT(pEye);
	_ASSERT(pViewProj);

	m_CameraEye = *pEye;
	m_CameraViewProjection = *pViewProj;
}

void Sun::createSunMesh(const int SEG_COUNT)
{
	_ASSERT(m_pRenderer);
	_ASSERT(!m_pSunDiskBuffer);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();

	m_SunDiskVertices.clear();

	m_SunDiskVertices.reserve(3 * SEG_COUNT);
	for (int i = 0; i < SEG_COUNT; ++i)
	{
		const float PHI_START = Lerp(0.0f, 2.0f * DirectX::XM_PI, (float)i / SEG_COUNT);
		const float PHI_END = Lerp(0.0f, 2.0f * DirectX::XM_PI, (float)(i + 1) / SEG_COUNT);

		const Vector2 A;
		const Vector2 B(cos(PHI_START), sin(PHI_START));
		const Vector2 C(cos(PHI_END), sin(PHI_END));

		m_SunDiskVertices.push_back(A);
		m_SunDiskVertices.push_back(B);
		m_SunDiskVertices.push_back(C);
	}

	BREAK_IF_FAILED(pResourceManager->CreateVertexBuffer(sizeof(Vector2), (UINT)m_SunDiskVertices.size(), &m_pSunDiskBuffer, m_SunDiskVertices.data()));
}

void Sun::createConstantBuffers()
{
	_ASSERT(m_pRenderer);
	_ASSERT(!m_pSunVSConstantBuffer);
	_ASSERT(!m_pSunPSConstantBuffer);

	ID3D11Device* pDevice = m_pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	m_pSunVSConstantBuffer = new ConstantBuffer;
	m_pSunVSConstantBuffer->Initialize(pDevice, pContext, sizeof(SunVSConstants), nullptr);

	m_pSunPSConstantBuffer = new ConstantBuffer;
	m_pSunPSConstantBuffer->Initialize(pDevice, pContext, sizeof(SunPSConstants), nullptr);
}
