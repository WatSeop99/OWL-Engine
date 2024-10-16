#include "../../Common.h"
#include "../Renderer/BaseRenderer.h"
#include "../Camera.h"
#include "../Renderer/ConstantBuffer.h"
#include "../ShadowMap.h"
#include "../Renderer/Texture.h"
#include "../Renderer/ResourceManager.h"
#include "Sun.h"

void Sun::Initialize(BaseRenderer* pRenderer, Camera* pMainCamera)
{
	_ASSERT(pRenderer);
	_ASSERT(pMainCamera);

	m_pRenderer = pRenderer;
	m_pMainCamera = pMainCamera;

	createSunMesh(SunDiskSegments);
	createConstantBuffers();

	SunProperty.Radius = 0.004649f;
	SunProperty.LightType = LIGHT_SUN | LIGHT_SHADOW;

	m_pSunCamera = new Camera;
	m_pSunCamera->bUseFirstPersonView = true;
	m_pSunCamera->SetAspectRatio(1.0f);
	m_pSunCamera->SetProjectionFovAngleY(120.0f);
	m_pSunCamera->SetNearZ(0.1f);
	m_pSunCamera->SetFarZ(1000.0f);

	//m_pSunShadowMap = new ShadowMap(3840, 3840);
	m_pSunShadowMap = new ShadowMap(5120, 5120);
	m_pSunShadowMap->Initialize(pRenderer, LIGHT_SUN | LIGHT_SHADOW);
}

void Sun::Update()
{
	_ASSERT(m_pSunVSConstantBuffer);
	_ASSERT(m_pSunPSConstantBuffer);

	// Update sun data.
	const float SUN_RAD_X = DegreeToRadian(SunAngle.x);
	const float SUN_RAD_Y = DegreeToRadian(-SunAngle.y);
	SunProperty.Direction = Vector3(cos(SUN_RAD_X) * cos(SUN_RAD_Y), sin(SUN_RAD_Y), sin(SUN_RAD_X) * cos(SUN_RAD_Y));
	SunProperty.Direction.Normalize();
	SunProperty.Position = -SunProperty.Direction * 30.0f;
	SunProperty.Radiance = SunIntensity * SunColor * 0.5f;
	SunViewProjection = Matrix::CreateLookAt(SunProperty.Position, Vector3::Zero, Vector3::UnitY) * Matrix::CreateOrthographic(20.0f, 20.0f, 0.1f, 80.0f);

	m_pSunCamera->SetEyePos(SunProperty.Position);
	m_pSunCamera->SetViewDir(SunProperty.Direction);

	// Update shadow.
	m_pSunShadowMap->Update(SunProperty, m_pSunCamera, m_pMainCamera);

	ConstantBuffer* const pShadowConstantBuffer = m_pSunShadowMap->GetShadowConstantBuffers();
	for (int i = 0; i < 4; ++i)
	{
		GlobalConstants* const pConstantData = (GlobalConstants*)pShadowConstantBuffer[i].pSystemMem;
		SunProperty.ViewProjections[i] = pConstantData->ViewProjection;
		SunProperty.Projections[i] = pConstantData->Projection;
		SunProperty.InverseProjections[i] = pConstantData->InverseProjection;
	}

	// Update VS constant buffer.
	SunVSConstants* pSunVSConstData = (SunVSConstants*)m_pSunVSConstantBuffer->pSystemMem;
	if (!pSunVSConstData)
	{
		__debugbreak();
	}

	Vector3 axisZ(SunProperty.Direction);
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

	const float SUN_THETA = asin(-SunProperty.Direction.y);
	SunWorld = Matrix::CreateScale(SunProperty.Radius) *
		Matrix(axisX, axisY, axisZ) *
		Matrix::CreateTranslation(m_CameraEye) *
		Matrix::CreateTranslation(-SunProperty.Direction);
	

	pSunVSConstData->WorldViewProjection = (SunWorld * m_CameraViewProjection).Transpose();
	pSunVSConstData->SunTheta = SUN_THETA;
	pSunVSConstData->EyeHeight = m_WorldScale * m_CameraEye.y;


	// Update PS constant buffer.
	SunPSConstants* pSunPSConstData = (SunPSConstants*)m_pSunPSConstantBuffer->pSystemMem;
	if (!pSunPSConstData)
	{
		__debugbreak();
	}
	pSunPSConstData->Radiance = SunProperty.Radiance;

	m_pSunVSConstantBuffer->Upload();
	m_pSunPSConstantBuffer->Upload();

	/*DirectX::SimpleMath::Vector4 eye(0.0f, 0.0f, 0.0f, 1.0f);
	DirectX::SimpleMath::Vector4 xLeft(-1.0f, -1.0f, 0.0f, 1.0f);
	DirectX::SimpleMath::Vector4 xRight(1.0f, 1.0f, 0.0f, 1.0f);
	Matrix lightProjection = Matrix::CreateOrthographic(20.0f, 20.0f, 0.1f, 80.0f);
	eye = DirectX::SimpleMath::Vector4::Transform(eye, lightProjection);
	xLeft = DirectX::SimpleMath::Vector4::Transform(xLeft, lightProjection.Invert());
	xRight = DirectX::SimpleMath::Vector4::Transform(xRight, lightProjection.Invert());
	xLeft /= xLeft.w;
	xRight /= xRight.w;
	std::cout << "LIGHT_FRUSTUM_WIDTH = " << xRight.x - xLeft.x << std::endl;*/
}

void Sun::Render()
{
	_ASSERT(m_pRenderer);
	_ASSERT(m_pSunDiskBuffer);
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
	if (!m_pSunDiskBuffer)
	{
		__debugbreak();
	}
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

void Sun::RenderShadowMap(std::vector<Model*>& basicList, Model* pMirror)
{
	_ASSERT(m_pSunShadowMap);

	m_pSunShadowMap->Render(basicList, pMirror);
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
	if (m_pSunCamera)
	{
		delete m_pSunCamera;
		m_pSunCamera = nullptr;
	}
	if (m_pSunShadowMap)
	{
		delete m_pSunShadowMap;
		m_pSunShadowMap = nullptr;
	}

	m_pRenderer = nullptr;
	m_pMainCamera = nullptr;
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

		const Vector2 A(cos(PHI_START), sin(PHI_START));
		const Vector2 B(cos(PHI_END), sin(PHI_END));

		m_SunDiskVertices.push_back(Vector2::Zero);
		m_SunDiskVertices.push_back(A);
		m_SunDiskVertices.push_back(B);
	}

	HRESULT hr = pResourceManager->CreateVertexBuffer(sizeof(Vector2), (UINT)m_SunDiskVertices.size(), &m_pSunDiskBuffer, m_SunDiskVertices.data());
	BREAK_IF_FAILED(hr);
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
