#include "../Common.h"
#include "Atmosphere/AerialLUT.h"
#include "../Renderer/BaseRenderer.h"
#include "Camera.h"
#include "../Geometry/GeometryGenerator.h"
#include "../Geometry/Mesh.h"
#include "../Geometry/Model.h"
#include "Atmosphere/MultiScatteringLUT.h"
#include "Atmosphere/Sky.h"
#include "Atmosphere/SkyLUT.h"
#include "Atmosphere/Sun.h"
#include "Atmosphere/TransmittanceLUT.h"
#include "Scene.h"

void Scene::Initialize(BaseRenderer* pRenderer, const bool bUSE_MSAA)
{
	_ASSERT(pRenderer);

	m_pRenderer = pRenderer;

	ResourceManager* pResourceManager = pRenderer->GetResourceManager();
	ID3D11Device* pDevice = pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = pRenderer->GetDeviceContext();

	LoadScene();

	createBuffers();
	m_GBuffer.bIsEnabled = true;
	if (m_GBuffer.bIsEnabled && !bUSE_MSAA)
	{
		m_GBuffer.Initialize(pDevice, pContext);
	}

	Lights.resize(MAX_LIGHTS);
	m_ppLightSpheres.resize(MAX_LIGHTS);

	// 조명 설정.
	{
		// 조명 0.
		Lights[0].Property.Radiance = Vector3(3.0f);
		Lights[0].Property.FallOffEnd = 10.0f;
		Lights[0].Property.Position = Vector3(0.0f);
		Lights[0].Property.Direction = Vector3(0.0f, 0.0f, 1.0f);
		Lights[0].Property.SpotPower = 3.0f;
		Lights[0].Property.LightType = LIGHT_POINT | LIGHT_SHADOW;
		Lights[0].Property.Radius = 0.04f;
		// Lights[0].Property.LightType = LIGHT_OFF;

		// 조명 1.
		Lights[1].Property.Radiance = Vector3(3.0f);
		Lights[1].Property.FallOffEnd = 10.0f;
		Lights[1].Property.Position = Vector3(1.0f, 1.1f, 2.0f);
		Lights[1].Property.SpotPower = 2.0f;
		Lights[1].Property.Direction = Vector3(0.0f, -0.5f, 1.7f) - Lights[1].Property.Position;
		Lights[1].Property.Direction.Normalize();
		Lights[1].Property.LightType = LIGHT_SPOT | LIGHT_SHADOW;
		Lights[1].Property.Radius = 0.02f;
		// Lights[1].Property.LightType = LIGHT_OFF;

		// 조명 2.
		Lights[2].Property.Radiance = Vector3(4.0f);
		Lights[2].Property.Position = Vector3(5.0f);
		Lights[2].Property.Direction = Vector3(-1.0f, -1.0f, -1.0f);
		Lights[2].Property.Direction.Normalize();
		Lights[2].Property.LightType = LIGHT_DIRECTIONAL | LIGHT_SHADOW;
		Lights[2].Property.Radius = 0.05f;
		// Lights[2].Property.LightType = LIGHT_OFF;
	}

	// 조명 위치 표시.
	{
		for (int i = 0; i < MAX_LIGHTS; ++i)
		{
			MeshInfo sphere;
			MakeSphere(&sphere, 1.0f, 20, 20);

			m_ppLightSpheres[i] = new Model;
			m_ppLightSpheres[i]->Initialize(pRenderer, { sphere });
			m_ppLightSpheres[i]->UpdateWorld(Matrix::CreateTranslation(Lights[i].Property.Position));

			MaterialConstants* pMaterialConstData = (MaterialConstants*)m_ppLightSpheres[i]->Meshes[0]->MaterialConstant.pSystemMem;
			pMaterialConstData->AlbedoFactor = Vector3(0.0f);
			pMaterialConstData->EmissionFactor = Vector3(1.0f, 1.0f, 0.0f);
			m_ppLightSpheres[i]->bCastShadow = false; // 조명 표시 물체들은 그림자 X.
			for (UINT64 j = 0, size = m_ppLightSpheres[i]->Meshes.size(); j < size; ++j)
			{
				Mesh* pCurMesh = m_ppLightSpheres[i]->Meshes[j];

				MaterialConstants* pMeshMaterialConstData = (MaterialConstants*)pCurMesh->MaterialConstant.pSystemMem;
				pMeshMaterialConstData->AlbedoFactor = Vector3(0.0f);
				pMeshMaterialConstData->EmissionFactor = Vector3(1.0f, 1.0f, 0.0f);
			}

			m_ppLightSpheres[i]->bIsVisible = true;
			m_ppLightSpheres[i]->Name = "LightSphere" + std::to_string(i);
			m_ppLightSpheres[i]->bIsPickable = false;

			RenderObjects.push_back(m_ppLightSpheres[i]); // 리스트에 등록.
		}
		// m_ppLightSpheres[0]->bIsVisible = false;
		// m_ppLightSpheres[1]->bIsVisible = false;
		// m_ppLightSpheres[2]->bIsVisible = false;
	}

	// 바닥(거울).
	{
		// https://freepbr.com/materials/stringy-marble-pbr/
		MeshInfo mesh;
		MakeSquare(&mesh, 10.0f);

		std::wstring path = L"./Assets/Textures/PBR/stringy-marble-ue/";
		mesh.szAlbedoTextureFileName = path + L"stringy_marble_albedo.png";
		mesh.szEmissiveTextureFileName = L"";
		mesh.szAOTextureFileName = path + L"stringy_marble_ao.png";
		mesh.szMetallicTextureFileName = path + L"stringy_marble_Metallic.png";
		mesh.szNormalTextureFileName = path + L"stringy_marble_Normal-dx.png";
		mesh.szRoughnessTextureFileName = path + L"stringy_marble_Roughness.png";

		m_pGround = new Model;
		m_pGround->Initialize(pRenderer, { mesh });

		MaterialConstants* pMatertialConstData = (MaterialConstants*)m_pGround->Meshes[0]->MaterialConstant.pSystemMem;
		pMatertialConstData->AlbedoFactor = Vector3(0.7f);
		pMatertialConstData->EmissionFactor = Vector3(0.0f);
		pMatertialConstData->MetallicFactor = 0.5f;
		pMatertialConstData->RoughnessFactor = 0.3f;

		// Vector3 position = Vector3(0.0f, -1.0f, 0.0f);
		Vector3 position = Vector3(0.0f, -0.5f, 0.0f);
		m_pGround->UpdateWorld(Matrix::CreateRotationX(DirectX::XM_PI * 0.5f) * Matrix::CreateTranslation(position));
		m_pGround->bCastShadow = false; // 바닥은 그림자 만들기 생략.

		m_MirrorPlane = DirectX::SimpleMath::Plane(position, Vector3(0.0f, 1.0f, 0.0f));
		m_pMirror = m_pGround; // 바닥에 거울처럼 반사 구현.
	}

	GlobalConstants initialGlobal;
	LightConstants initialLight;
	m_GlobalConstants.Initialize(pDevice, pContext, sizeof(GlobalConstants), &initialGlobal);
	m_ReflectionGlobalConstants.Initialize(pDevice, pContext, sizeof(GlobalConstants), &initialGlobal);
	m_LightConstants.Initialize(pDevice, pContext, sizeof(LightConstants), &initialLight);

	GlobalConstants* pGlobalConstData = (GlobalConstants*)m_GlobalConstants.pSystemMem;
	pGlobalConstData->StrengthIBL = 0.2f;
	//m_GlobalConstants.CPU.StrengthIBL = 0.1f;
	// m_GlobalConstants.CPU.StrengthIBL = 1.0f;

	// 광원마다 shadow map 설정.
	for (UINT64 i = 0, size = Lights.size(); i < size; ++i)
	{
		// Lights[i].SetShadowSize(1280, 1280); // 사이즈 변경 가능.
		Lights[i].Initialize(pRenderer);
	}

	// 환경 박스 초기화.
	MeshInfo skyboxMeshInfo;
	MakeBox(&skyboxMeshInfo, 40.0f);

	std::reverse(skyboxMeshInfo.Indices.begin(), skyboxMeshInfo.Indices.end());
	m_pSkybox = new Model;
	m_pSkybox->Initialize(pRenderer, { skyboxMeshInfo });
	m_pSkybox->Name = "SkyBox";

	// deferred rendering을 위한 스크린 공간 생성.
	if (m_GBuffer.bIsEnabled)
	{
		MeshInfo meshInfo;
		MakeSquare(&meshInfo);

		m_pScreenMesh = new Mesh;
		m_pScreenMesh->Initialize(pDevice, pContext);

		HRESULT hr = pResourceManager->CreateVertexBuffer(sizeof(Vertex), (UINT)meshInfo.Vertices.size(), &m_pScreenMesh->pVertexBuffer, meshInfo.Vertices.data());
		BREAK_IF_FAILED(hr);

		m_pScreenMesh->IndexCount = (UINT)meshInfo.Indices.size();
		hr = pResourceManager->CreateIndexBuffer(sizeof(UINT), (UINT)meshInfo.Indices.size(), &m_pScreenMesh->pIndexBuffer, meshInfo.Indices.data());
		BREAK_IF_FAILED(hr);
	}


	m_SunProperty.LightType = LIGHT_SUN | LIGHT_SHADOW;
	m_SunCamera.bUseFirstPersonView = true;
	m_SunCamera.SetAspectRatio(2560.0f / 2560.0f);
	m_SunCamera.SetEyePos(m_SunProperty.Position);
	m_SunCamera.SetViewDir(m_SunProperty.Direction);
	m_SunCamera.SetProjectionFovAngleY(120.0f);
	m_SunCamera.SetNearZ(0.1f);
	m_SunCamera.SetFarZ(50.0f);
	m_ShadowMap.SetShadowWidth(2560);
	m_ShadowMap.SetShadowHeight(2560);
	m_ShadowMap.Initialize(pRenderer, LIGHT_SUN | LIGHT_SHADOW);

	AtmosphereProperty STDUnitAtmosphereProperty = m_AtmosphereProperty.ToStdUnit();
	m_pAtmosphereConstantBuffer = new ConstantBuffer;
	m_pAtmosphereConstantBuffer->Initialize(pDevice, pContext, sizeof(AtmosphereProperty), &STDUnitAtmosphereProperty);
	m_pSky = new Sky;
	m_pSky->Initialize(pRenderer);
	m_pSkyLUT = new SkyLUT;
	m_pSkyLUT->Initialize(pRenderer);
	m_pAerialLUT = new AerialLUT;
	m_pAerialLUT->Initialize(pRenderer);
	m_pTransmittanceLUT = new TransmittanceLUT;
	m_pTransmittanceLUT->Initialize(pRenderer);
	m_pMultiScatteringLUT = new MultiScatteringLUT;
	m_pMultiScatteringLUT->Initialize(pRenderer);
	m_pSun = new Sun;
	m_pSun->Initialize(pRenderer);

	m_pTransmittanceLUT->SetAtmosphere(m_pAtmosphereConstantBuffer);
	m_pTransmittanceLUT->Generate();

	Vector3 terrainColor(0.3f);
	m_pMultiScatteringLUT->SetAtmosphere(m_pAtmosphereConstantBuffer);
	m_pMultiScatteringLUT->Update(&terrainColor);
	m_pMultiScatteringLUT->Generate(m_pTransmittanceLUT->GetTransmittanceLUT());

	m_pSkyLUT->SetAtmosphere(m_pAtmosphereConstantBuffer);
	m_pSkyLUT->SetMultiScatteringLUT(m_pMultiScatteringLUT->GetMultiScatteringLUT());
	m_pSkyLUT->SetTransmittanceLUT(m_pTransmittanceLUT->GetTransmittanceLUT());

	m_pAerialLUT->SetAtmosphere(m_pAtmosphereConstantBuffer);
	m_pAerialLUT->SetMultiScatteringLUT(m_pMultiScatteringLUT->GetMultiScatteringLUT());
	m_pAerialLUT->SetTransmittanceLUT(m_pTransmittanceLUT->GetTransmittanceLUT());
	m_pAerialLUT->SetShadow(&m_ShadowMap);

	m_pSun->SetAtmosphere(m_pAtmosphereConstantBuffer);
	m_pSun->SetTransmittanceLUT(m_pTransmittanceLUT->GetTransmittanceLUT());
	m_pSun->Update();
}

void Scene::Update(const float DELTA_TIME)
{
	updateLights(DELTA_TIME);
	updateGlobalConstants(DELTA_TIME);

	const Vector3 CAMERA_POS = m_Camera.GetEyePos();
	const Matrix CAMERA_VIEWPROJECTION = m_Camera.GetView() * m_Camera.GetProjection();
	const float ATMOS_EYE_HEIGHT = m_pAerialLUT->pAerialData->WorldScale * CAMERA_POS.y;
	const FrustumDirection CAMERA_FRUSTUM = m_Camera.GetFrustumDirection();

	// Update Sun predata.
	// m_pSun->SunAngle.x = Clamp(sin(m_pSun->SunAngle.x + DELTA_TIME), 0.0f, 360.0f);
	m_pSun->SunAngle.y = 12.0f;

	m_pSun->SetCamera(&CAMERA_POS, &CAMERA_VIEWPROJECTION);
	m_pSun->Update();

	// Update shadow map.
	m_SunProperty.Radiance = m_pSun->SunRadiance;
	m_SunProperty.Direction = m_pSun->SunDirection;
	m_SunProperty.Position = m_pSun->SunWorld.Translation();
	m_SunProperty.Radius = m_pSun->SunRadius;
	m_ShadowMap.Update(m_SunProperty, m_SunCamera, m_Camera);

	// Update aerial LUT.
	m_pAerialLUT->SetCamera(&CAMERA_POS, ATMOS_EYE_HEIGHT, &CAMERA_FRUSTUM);
	m_pAerialLUT->SetSun(&m_pSun->SunDirection);
	m_pAerialLUT->Update();

	// Update sky view LUT.
	m_pSkyLUT->SetCamera(&(CAMERA_POS * m_pAerialLUT->pAerialData->WorldScale));
	m_pSkyLUT->SetSun(&m_pSun->SunDirection, &(m_pSun->SunIntensity * m_pSun->SunColor));
	m_pSkyLUT->Update();

	// Update Sky.
	m_pSky->SetCamera(&CAMERA_FRUSTUM);
	m_pSky->Update();

	// Update Sun.
	m_pSun->SetWorldScale(m_pAerialLUT->pAerialData->WorldScale);


	if (m_pMirror)
	{
		m_pMirror->UpdateConstantBuffers();
	}

	for (UINT64 i = 0, size = RenderObjects.size(); i < size; ++i)
	{
		Model* const pCurModel = RenderObjects[i];
		pCurModel->UpdateConstantBuffers();
	}
}

void Scene::Render()
{
	_ASSERT(m_pRenderer);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	setScreenViewport();

	// 공통으로 사용하는 샘플러들 설정.
	pContext->VSSetSamplers(0, (UINT)pResourceManager->SamplerStates.size(), pResourceManager->SamplerStates.data());
	pContext->PSSetSamplers(0, (UINT)pResourceManager->SamplerStates.size(), pResourceManager->SamplerStates.data());

	// 공통으로 사용할 텍스춰들: "Common.hlsli"에서 register(t10)부터 시작
	ID3D11ShaderResourceView* ppCommonSRVs[] = { m_pEnvSRV, m_pSpecularSRV, m_pIrradianceSRV, m_pBrdfSRV };
	UINT numCommonSRVs = _countof(ppCommonSRVs);
	pContext->PSSetShaderResources(10, numCommonSRVs, ppCommonSRVs);

	renderDepthOnly();
	renderShadowMaps();

	m_pSkyLUT->Generate();
	m_pAerialLUT->Generate();

	if (m_GBuffer.bIsEnabled)
	{
		renderGBuffer();
		renderDeferredLighting();
	}
	else
	{
		renderOpaqueObjects();
	}

	renderMirror();
	renderSky();
	renderOptions();
}

void Scene::Cleanup()
{
	// SaveScene();

	if (m_pAtmosphereConstantBuffer)
	{
		delete m_pAtmosphereConstantBuffer;
		m_pAtmosphereConstantBuffer = nullptr;
	}
	if (m_pSky)
	{
		delete m_pSky;
		m_pSky = nullptr;
	}
	if (m_pSkyLUT)
	{
		delete m_pSkyLUT;
		m_pSkyLUT = nullptr;
	}
	if (m_pAerialLUT)
	{
		delete m_pAerialLUT;
		m_pAerialLUT = nullptr;
	}
	if (m_pTransmittanceLUT)
	{
		delete m_pTransmittanceLUT;
		m_pTransmittanceLUT = nullptr;
	}
	if (m_pMultiScatteringLUT)
	{
		delete m_pMultiScatteringLUT;
		m_pMultiScatteringLUT = nullptr;
	}
	if (m_pSun)
	{
		delete m_pSun;
		m_pSun = nullptr;
	}


	m_pMirror = nullptr;
	m_ppLightSpheres.clear();
	if (m_pGround)
	{
		delete m_pGround;
		m_pGround = nullptr;
	}
	if (m_pSkybox)
	{
		delete m_pSkybox;
		m_pSkybox = nullptr;
	}

	SAFE_RELEASE(m_pBrdfSRV);
	SAFE_RELEASE(m_pSpecularSRV);
	SAFE_RELEASE(m_pIrradianceSRV);
	SAFE_RELEASE(m_pEnvSRV);

	SAFE_RELEASE(m_pDefaultDSV);

	if (m_pScreenMesh)
	{
		delete m_pScreenMesh;
		m_pScreenMesh = nullptr;
	}

	Lights.clear();
	for (UINT64 i = 0, size = RenderObjects.size(); i < size; ++i)
	{
		delete RenderObjects[i];
		RenderObjects[i] = nullptr;
	}
	RenderObjects.clear();

	m_pRenderer = nullptr;
	m_pFloatBuffer = nullptr;
	m_pResolvedBuffer = nullptr;
}

void Scene::ResetBuffers(const bool bUSE_MSAA, const UINT NUM_QUALITY_LEVELS)
{
	_ASSERT(m_pRenderer);

	ID3D11Device* pDevice = m_pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	SAFE_RELEASE(m_pDefaultDSV);
	m_DepthOnlyBuffer.Cleanup();

	if (m_GBuffer.bIsEnabled && !bUSE_MSAA)
	{
		m_GBuffer.Cleanup();
		m_GBuffer.SetScreenWidth(m_ScreenWidth);
		m_GBuffer.SetScreenHeight(m_ScreenHeight);
		m_GBuffer.Initialize(pDevice, pContext);
	}

	createDepthBuffers(bUSE_MSAA, NUM_QUALITY_LEVELS);
}

void Scene::initCubemaps(std::wstring&& basePath, std::wstring&& envFileName, std::wstring&& specularFileName, std::wstring&& irradianceFileName, std::wstring&& brdfFileName)
{
	_ASSERT(m_pRenderer);

	HRESULT hr = S_OK;
	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11Device* pDevice = m_pRenderer->GetDevice();

	D3D11_TEXTURE2D_DESC textureDesc = {};
	ID3D11Texture2D* pTexture = nullptr;

	// BRDF LookUp Table은 CubeMap이 아니라 2D 텍스쳐임.
	hr = pResourceManager->CreateTextureCubeFromFile((basePath + envFileName).c_str(), &pTexture, &textureDesc);
	BREAK_IF_FAILED(hr);
	hr = pDevice->CreateShaderResourceView(pTexture, nullptr, &m_pEnvSRV);
	BREAK_IF_FAILED(hr);
	pTexture->Release();
	pTexture = nullptr;
	/*D3D11_TEXTURE2D_DESC textureDesc = {};
	m_pEnv = new Texture;
	pResourceManager->CreateTextureCubeFromFile((basePath + envFileName).c_str(), m_pEnv->GetTexture2DPtr(), &textureDesc);*/

	hr = pResourceManager->CreateTextureCubeFromFile((basePath + specularFileName).c_str(), &pTexture, &textureDesc);
	BREAK_IF_FAILED(hr);
	hr = pDevice->CreateShaderResourceView(pTexture, nullptr, &m_pSpecularSRV);
	BREAK_IF_FAILED(hr);
	pTexture->Release();
	pTexture = nullptr;
	/*m_pSpecular = new Texture;
	pResourceManager->CreateTextureCubeFromFile((basePath + specularFileName).c_str(), m_pSpecular->GetTexture2DPtr(), &textureDesc);*/

	hr = pResourceManager->CreateTextureCubeFromFile((basePath + irradianceFileName).c_str(), &pTexture, &textureDesc);
	BREAK_IF_FAILED(hr);
	hr = pDevice->CreateShaderResourceView(pTexture, nullptr, &m_pIrradianceSRV);
	BREAK_IF_FAILED(hr);
	pTexture->Release();
	pTexture = nullptr;
	/*m_pIrradiance = new Texture;
	pResourceManager->CreateTextureCubeFromFile((basePath + irradianceFileName).c_str(), m_pIrradiance->GetTexture2DPtr(), &textureDesc);*/

	hr = pResourceManager->CreateTextureCubeFromFile((basePath + brdfFileName).c_str(), &pTexture, &textureDesc);
	BREAK_IF_FAILED(hr);
	hr = pDevice->CreateShaderResourceView(pTexture, nullptr, &m_pBrdfSRV);
	BREAK_IF_FAILED(hr);
	pTexture->Release();
	pTexture = nullptr;
	/*m_pBRDF = new Texture;
	pResourceManager->CreateTextureCubeFromFile((basePath + brdfFileName).c_str(), m_pBRDF->GetTexture2DPtr(), &textureDesc);*/
}

void Scene::createBuffers(const bool bUSE_MSAA, const UINT NUM_QUALITY_LEVELS)
{
	createDepthBuffers(bUSE_MSAA, NUM_QUALITY_LEVELS);
}

void Scene::createDepthBuffers(const bool bUSE_MSAA, const UINT NUM_QUALITY_LEVELS)
{
	_ASSERT(m_pRenderer);

	HRESULT hr = S_OK;
	ID3D11Device* pDevice = m_pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	// depth buffers.
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = m_ScreenWidth;
	desc.Height = m_ScreenHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	if (!m_GBuffer.bIsEnabled && bUSE_MSAA && NUM_QUALITY_LEVELS > 0)
	{
		desc.SampleDesc.Count = 4;
		desc.SampleDesc.Quality = NUM_QUALITY_LEVELS - 1;
	}
	else
	{
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
	}
	desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // DSV 기준.

	ID3D11Texture2D* pDepthStencilBuffer = nullptr;
	hr = pDevice->CreateTexture2D(&desc, nullptr, &pDepthStencilBuffer);
	BREAK_IF_FAILED(hr);

	if (!m_GBuffer.bIsEnabled && bUSE_MSAA && NUM_QUALITY_LEVELS > 0)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
		hr = pDevice->CreateDepthStencilView(pDepthStencilBuffer, &dsvDesc, &m_pDefaultDSV);
	}
	else
	{
		hr = pDevice->CreateDepthStencilView(pDepthStencilBuffer, nullptr, &m_pDefaultDSV);
	}
	RELEASE(pDepthStencilBuffer);
	BREAK_IF_FAILED(hr);

	// Depth 전용.
	desc.Format = DXGI_FORMAT_D32_FLOAT; // DSV 기준 포맷.
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	m_DepthOnlyBuffer.Initialize(pDevice, pContext, desc, nullptr, true);
}

void Scene::updateLights(const float DELTA_TIME)
{
	for (UINT64 i = 0, size = Lights.size(); i < size; ++i)
	{
		Lights[i].Update(DELTA_TIME, m_Camera);
		m_ppLightSpheres[i]->UpdateWorld(Matrix::CreateScale(Max(0.01f, Lights[i].Property.Radius)) * Matrix::CreateTranslation(Lights[i].Property.Position));
		memcpy(&((LightConstants*)m_LightConstants.pSystemMem)->Lights[i], &Lights[i].Property, sizeof(LightProperty));
	}

	m_LightConstants.Upload();
}

void Scene::updateGlobalConstants(const float DELTA_TIME)
{
	const Vector3 EYE_WORLD = m_Camera.GetEyePos();
	const Matrix REFLECTION = Matrix::CreateReflection(m_MirrorPlane);
	const Matrix VIEW = m_Camera.GetView();
	const Matrix PROJECTION = m_Camera.GetProjection();

	GlobalConstants* pGlobalConstData = (GlobalConstants*)m_GlobalConstants.pSystemMem;
	GlobalConstants* pReflectionConstData = (GlobalConstants*)m_ReflectionGlobalConstants.pSystemMem;

	pGlobalConstData->GlobalTime += DELTA_TIME;
	pGlobalConstData->EyeWorld = EYE_WORLD;
	pGlobalConstData->View = VIEW.Transpose();
	pGlobalConstData->Projection = PROJECTION.Transpose();
	pGlobalConstData->InverseProjection = PROJECTION.Invert().Transpose();
	pGlobalConstData->ViewProjection = (VIEW * PROJECTION).Transpose();
	pGlobalConstData->InverseView = VIEW.Invert().Transpose();

	// 그림자 렌더링에 사용.
	pGlobalConstData->InverseViewProjection = pGlobalConstData->ViewProjection.Invert();

	memcpy(pReflectionConstData, pGlobalConstData, sizeof(GlobalConstants));
	pReflectionConstData->View = (REFLECTION * VIEW).Transpose();
	pReflectionConstData->ViewProjection = (REFLECTION * VIEW * PROJECTION).Transpose();
	// 그림자 렌더링에 사용 (광원의 위치도 반사시킨 후에 계산해야 함).
	pReflectionConstData->InverseViewProjection = pReflectionConstData->ViewProjection.Invert();

	m_GlobalConstants.Upload();
	m_ReflectionGlobalConstants.Upload();
}

void Scene::renderDepthOnly()
{
	_ASSERT(m_pRenderer);
	
	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	pContext->OMSetRenderTargets(0, nullptr, m_DepthOnlyBuffer.pDSV);
	pContext->ClearDepthStencilView(m_DepthOnlyBuffer.pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	setGlobalConstants(&m_GlobalConstants.pBuffer, 0);
	//setPipelineState(g_DepthOnlyPSO);
	pResourceManager->SetPipelineState(GraphicsPSOType_DepthOnly);

	for (UINT64 i = 0, size = RenderObjects.size(); i < size; ++i)
	{
		Model* pCurModel = RenderObjects[i];
		pCurModel->Render();
	}

	/*if (m_pSkybox)
	{
		m_pSkybox->Render();
	}*/
	if (m_pMirror)
	{
		m_pMirror->Render();
	}
}

void Scene::renderShadowMaps()
{
	_ASSERT(m_pRenderer);

	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	// 쉐도우 맵을 다른 쉐이더에서 SRV 해제.
	ID3D11ShaderResourceView* ppNulls[3] = { nullptr, };
	pContext->PSSetShaderResources(14, 3, ppNulls);

	 for (UINT64 i = 0, size = Lights.size(); i < size; ++i)
	 {
	 	Lights[i].RenderShadowMap(RenderObjects, m_pMirror);
	 }
	m_ShadowMap.Render(RenderObjects, m_pMirror);
}

void Scene::renderSky()
{
	_ASSERT(m_pRenderer);
	_ASSERT(m_pSky);
	_ASSERT(m_pSun);
	_ASSERT(m_pSkyLUT);

	m_pSky->Render(m_pSkyLUT->GetSkyLUT());
	m_pSun->Render();
}

void Scene::renderOpaqueObjects()
{
	_ASSERT(m_pRenderer);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	// 다시 렌더링 해상도로 되돌리기.
	setScreenViewport();

	// 거울은 빼고 원래 대로 그리기.
	const float CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	pContext->ClearRenderTargetView(m_pFloatBuffer->pRTV, CLEAR_COLOR);
	pContext->ClearDepthStencilView(m_pDefaultDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	pContext->OMSetRenderTargets(1, &m_pFloatBuffer->pRTV, m_pDefaultDSV);

	// 그림자맵들도 공용 텍스춰들 이후에 추가.
	// 주의: 마지막 shadowDSV를 RenderTarget에서 해제한 후 설정.
	ID3D11ShaderResourceView* ppShadowSRVs[MAX_LIGHTS] = { nullptr, };
	UINT pointLightIndex = -1;
	UINT directionalLightIndex = -1;
	for (UINT64 i = 0, size = Lights.size(); i < size; ++i)
	{
		ShadowMap& curShadowMap = Lights[i].GetShadowMap();

		switch (Lights[i].Property.LightType & (LIGHT_DIRECTIONAL | LIGHT_POINT | LIGHT_SPOT))
		{
			case LIGHT_DIRECTIONAL:
				directionalLightIndex = (UINT)i;
				break;

			case LIGHT_POINT:
				pointLightIndex = (UINT)i;
				break;

			case LIGHT_SPOT:
			{
				Texture* pCurShadowBuffer = curShadowMap.GetSpotLightShadowBuffer();
				ppShadowSRVs[i] = pCurShadowBuffer->pSRV;
			}
			break;

			default:
				break;
		}
	}
	pContext->PSSetShaderResources(14, (UINT)Lights.size(), ppShadowSRVs);

	if (pointLightIndex != -1)
	{
		ShadowMap& curShadowMap = Lights[pointLightIndex].GetShadowMap();
		Texture* pCurShadowCubeBuffer = curShadowMap.GetPointLightShadowBuffer();
		pContext->PSSetShaderResources(17, 1, &pCurShadowCubeBuffer->pSRV);
	}
	if (directionalLightIndex != -1)
	{
		ShadowMap& curShadowMap = Lights[directionalLightIndex].GetShadowMap();
		Texture* pCurShadowCascadeBuffer = curShadowMap.GetDirectionalLightShadowBuffer();
		pContext->PSSetShaderResources(18, 1, &pCurShadowCascadeBuffer->pSRV);
	}

	setGlobalConstants(&m_GlobalConstants.pBuffer, 0);
	setGlobalConstants(&m_LightConstants.pBuffer, 1);

	// 스카이박스 그리기.
	// 최적화를 하고 싶다면 투명한 물체들만 따로 마지막에 그리면 됨.
	/*pResourceManager->SetPipelineState(bDrawAsWire ? GraphicsPSOType_SkyboxWire : GraphicsPSOType_SkyboxSolid);
	m_pSkybox->Render();*/

	for (UINT64 i = 0, size = RenderObjects.size(); i < size; ++i)
	{
		Model* pCurModel = RenderObjects[i];
		pResourceManager->SetPipelineState(pCurModel->GetPSO(bDrawAsWire));
		pCurModel->Render();
	}
}

void Scene::renderGBuffer()
{
	_ASSERT(m_pRenderer);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();

	// 다시 렌더링 해상도로 되돌리기.
	setScreenViewport();

	setGlobalConstants(&m_GlobalConstants.pBuffer, 0);

	m_GBuffer.PrepareRender(m_pDefaultDSV);

	for (UINT64 i = 0, size = RenderObjects.size(); i < size; ++i)
	{
		Model* pCurModel = RenderObjects[i];
		pResourceManager->SetPipelineState(pCurModel->GetGBufferPSO(bDrawAsWire));
		pCurModel->Render();
	}

	m_GBuffer.AfterRender();
}

void Scene::renderDeferredLighting()
{
	_ASSERT(m_pRenderer);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	// 그림자맵들도 공용 텍스춰들 이후에 추가.
	ID3D11ShaderResourceView* ppShadowSRVs[MAX_LIGHTS] = { nullptr, };
	UINT pointLightIndex = -1;
	UINT directionalLightIndex = -1;
	for (UINT64 i = 0, size = Lights.size(); i < size; ++i)
	{
		ShadowMap& curShadowMap = Lights[i].GetShadowMap();
		switch (Lights[i].Property.LightType & (LIGHT_DIRECTIONAL | LIGHT_POINT | LIGHT_SPOT))
		{
			case LIGHT_DIRECTIONAL:
				directionalLightIndex = (UINT)i;
				break;

			case LIGHT_POINT:
				pointLightIndex = (UINT)i;
				break;

			case LIGHT_SPOT:
			{
				Texture* pCurShadowBuffer = curShadowMap.GetSpotLightShadowBuffer();
				ppShadowSRVs[i] = pCurShadowBuffer->pSRV;
			}
			break;

			default:
				break;
		}
	}
	pContext->PSSetShaderResources(14, (UINT)Lights.size(), ppShadowSRVs);

	if (pointLightIndex != -1)
	{
		ShadowMap& curShadowMap = Lights[pointLightIndex].GetShadowMap();
		Texture* pCurShadowCubeBuffer = curShadowMap.GetPointLightShadowBuffer();
		pContext->PSSetShaderResources(17, 1, &pCurShadowCubeBuffer->pSRV);
	}
	if (directionalLightIndex != -1)
	{
		ShadowMap& curShadowMap = Lights[directionalLightIndex].GetShadowMap();
		Texture* pCurShadowCascadeBuffer = curShadowMap.GetDirectionalLightShadowBuffer();
		pContext->PSSetShaderResources(18, 1, &pCurShadowCascadeBuffer->pSRV);
	}

	setGlobalConstants(&m_LightConstants.pBuffer, 1);

	const float CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	pContext->ClearRenderTargetView(m_pFloatBuffer->pRTV, CLEAR_COLOR);
	pContext->ClearDepthStencilView(m_pDefaultDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	pContext->OMSetRenderTargets(1, &m_pFloatBuffer->pRTV, m_pDefaultDSV);

	ID3D11ShaderResourceView* ppSRVs[5] = { m_GBuffer.AlbedoBuffer.pSRV, m_GBuffer.NormalBuffer.pSRV, m_GBuffer.PositionBuffer.pSRV, m_GBuffer.EmissionBuffer.pSRV, m_GBuffer.ExtraBuffer.pSRV };
	pContext->PSSetShaderResources(0, 5, ppSRVs);

	pResourceManager->SetPipelineState(GraphicsPSOType_DeferredRendering);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &m_pScreenMesh->pVertexBuffer, &stride, &offset);
	pContext->IASetIndexBuffer(m_pScreenMesh->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	pContext->DrawIndexed(m_pScreenMesh->IndexCount, 0, 0);

	pContext->OMSetRenderTargets(1, &m_pFloatBuffer->pRTV, m_GBuffer.DepthBuffer.pDSV);

	// 스카이박스 그리기.
	/*pResourceManager->SetPipelineState(bDrawAsWire ? GraphicsPSOType_SkyboxWire : GraphicsPSOType_SkyboxSolid);
	m_pSkybox->Render();*/
}

void Scene::renderOptions()
{
	_ASSERT(m_pRenderer);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();

	setGlobalConstants(&m_GlobalConstants.pBuffer, 0);

	for (UINT64 i = 0, size = RenderObjects.size(); i < size; ++i)
	{
		Model* pCurModel = RenderObjects[i];
		if (pCurModel->bDrawNormals)
		{
			pResourceManager->SetPipelineState(GraphicsPSOType_Normal);
			pCurModel->RenderNormals();
		}
		if (bDrawOBB)
		{
			pResourceManager->SetPipelineState(GraphicsPSOType_BoundingBox);
			pCurModel->RenderWireBoundingBox();
		}
		if (bDrawBS)
		{
			pResourceManager->SetPipelineState(GraphicsPSOType_BoundingBox);
			pCurModel->RenderWireBoundingSphere();
		}
	}
}

void Scene::renderMirror()
{
	_ASSERT(m_pRenderer);

	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	if (!m_pMirror)
	{
		return;
	}

	if (MirrorAlpha == 1.0f) // 불투명하면 거울만 그림.
	{
		pResourceManager->SetPipelineState(bDrawAsWire ? GraphicsPSOType_DefaultWire : GraphicsPSOType_DefaultSolid);
		m_pMirror->Render();
	}
	else if (MirrorAlpha < 1.0f) // 투명도가 조금이라도 있으면 처리.
	{
		// 거울 위치만 StencilBuffer에 1로 표기.
		pResourceManager->SetPipelineState(GraphicsPSOType_StencilMask);
		m_pMirror->Render();

		// 거울 위치에 반사된 물체들을 렌더링.
		setGlobalConstants(&m_ReflectionGlobalConstants.pBuffer, 0);
		pContext->ClearDepthStencilView(m_pDefaultDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

		for (size_t i = 0, size = RenderObjects.size(); i < size; ++i)
		{
			Model* pCurModel = RenderObjects[i];
			pResourceManager->SetPipelineState(pCurModel->GetPSO(bDrawAsWire));
			pCurModel->Render();
		}

		/*pResourceManager->SetPipelineState(bDrawAsWire ? GraphicsPSOType_ReflectSkyboxWire : GraphicsPSOType_ReflectSkyboxSolid);
		m_pSkybox->Render();*/

		// 거울 자체의 재질을 "Blend"로 그림.
		setGlobalConstants(&m_GlobalConstants.pBuffer, 0);
		pResourceManager->SetPipelineState(bDrawAsWire ? GraphicsPSOType_MirrorBlendWire : GraphicsPSOType_MirrorBlendSolid);
		m_pMirror->Render();
	}
}

void Scene::setScreenViewport()
{
	_ASSERT(m_pRenderer);

	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	D3D11_VIEWPORT screenViewport = { 0, };
	screenViewport.TopLeftX = 0;
	screenViewport.TopLeftY = 0;
	screenViewport.Width = (float)m_ScreenWidth;
	screenViewport.Height = (float)m_ScreenHeight;
	screenViewport.MinDepth = 0.0f;
	screenViewport.MaxDepth = 1.0f;

	pContext->RSSetViewports(1, &screenViewport);
}

void Scene::setGlobalConstants(ID3D11Buffer** ppGlobalConstants, UINT slot)
{
	_ASSERT(m_pRenderer);

	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	// 쉐이더와 일관성 유지 cbuffer GlobalConstants : register(slot).
	pContext->VSSetConstantBuffers(slot, 1, ppGlobalConstants);
	pContext->PSSetConstantBuffers(slot, 1, ppGlobalConstants);
	pContext->GSSetConstantBuffers(slot, 1, ppGlobalConstants);
}
