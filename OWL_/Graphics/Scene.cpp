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

void Scene::Initialize(BaseRenderer* pRenderer)
{
	_ASSERT(pRenderer);

	m_pRenderer = pRenderer;

	ResourceManager* pResourceManager = pRenderer->GetResourceManager();
	ID3D11Device* pDevice = pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = pRenderer->GetDeviceContext();

	LoadScene();

	Lights.resize(3);
	m_ppLightSpheres.resize(3);

	// 조명 설정.
	{
		// 조명 0.
		Lights[0].Property.Radiance = Vector3(1.0f);
		Lights[0].Property.FallOffEnd = 10.0f;
		Lights[0].Property.Position = Vector3(0.0f);
		Lights[0].Property.Direction = Vector3(0.0f, 0.0f, 1.0f);
		Lights[0].Property.SpotPower = 3.0f;
		Lights[0].Property.LightType = LIGHT_POINT | LIGHT_SHADOW;
		Lights[0].Property.Radius = 0.04f;
		Lights[0].Property.LightType = LIGHT_OFF;

		// 조명 1.
		Lights[1].Property.Radiance = Vector3(1.0f);
		Lights[1].Property.FallOffEnd = 10.0f;
		Lights[1].Property.Position = Vector3(1.0f, 1.1f, 2.0f);
		Lights[1].Property.SpotPower = 2.0f;
		Lights[1].Property.Direction = Vector3(0.0f, -0.5f, 1.7f) - Lights[1].Property.Position;
		Lights[1].Property.Direction.Normalize();
		Lights[1].Property.LightType = LIGHT_SPOT | LIGHT_SHADOW;
		Lights[1].Property.Radius = 0.02f;
		Lights[1].Property.LightType = LIGHT_OFF;

		// 조명 2.
		Lights[2].Property.Radiance = Vector3(1.0f);
		Lights[2].Property.Position = Vector3(4.0f);
		Lights[2].Property.Direction = Vector3(-1.0f, -1.0f, -1.0f);
		Lights[2].Property.Direction.Normalize();
		Lights[2].Property.LightType = LIGHT_DIRECTIONAL | LIGHT_SHADOW;
		Lights[2].Property.Radius = 0.05f;
		Lights[2].Property.LightType = LIGHT_OFF;
	}

	// 조명 위치 표시.
	{
		for (int i = 0; i < 3; ++i)
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
		RenderObjects.push_back(m_pGround);

		m_MirrorPlane = DirectX::SimpleMath::Plane(position, Vector3(0.0f, 1.0f, 0.0f));
		m_pMirror = m_pGround; // 바닥에 거울처럼 반사 구현.
	}

	GlobalConstants initialGlobal;
	LightConstants initialLight;
	m_GlobalConstants.Initialize(pDevice, pContext, sizeof(GlobalConstants), &initialGlobal);
	m_ReflectionGlobalConstants.Initialize(pDevice, pContext, sizeof(GlobalConstants), &initialGlobal);
	m_LightConstants.Initialize(pDevice, pContext, sizeof(LightConstants), &initialLight);

	GlobalConstants* pGlobalConstData = (GlobalConstants*)m_GlobalConstants.pSystemMem;
	if (!pGlobalConstData)
	{
		__debugbreak();
	}
	pGlobalConstData->StrengthIBL = 0.2f;
	//m_GlobalConstants.CPU.StrengthIBL = 0.1f;
	//m_GlobalConstants.CPU.StrengthIBL = 1.0f;

	// 광원마다 shadow map 설정.
	for (UINT64 i = 0, size = Lights.size(); i < size; ++i)
	{
		Lights[i].Initialize(pRenderer);
	}

	// 환경 박스 초기화.
	MeshInfo skyboxMeshInfo;
	MakeBox(&skyboxMeshInfo, 40.0f);

	std::reverse(skyboxMeshInfo.Indices.begin(), skyboxMeshInfo.Indices.end());
	m_pSkybox = new Model;
	m_pSkybox->Initialize(pRenderer, { skyboxMeshInfo });
	m_pSkybox->Name = "SkyBox";


	/*m_SunProperty.LightType = LIGHT_SUN | LIGHT_SHADOW;
	m_SunCamera.bUseFirstPersonView = true;
	m_SunCamera.SetAspectRatio(2560.0f / 2560.0f);
	m_SunCamera.SetEyePos(m_SunProperty.Position);
	m_SunCamera.SetViewDir(m_SunProperty.Direction);
	m_SunCamera.SetProjectionFovAngleY(120.0f);
	m_SunCamera.SetNearZ(0.1f);
	m_SunCamera.SetFarZ(50.0f);
	m_ShadowMap.SetShadowWidth(2560);
	m_ShadowMap.SetShadowHeight(2560);
	m_ShadowMap.Initialize(pRenderer, LIGHT_SUN | LIGHT_SHADOW);*/

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
	m_pSun->Initialize(pRenderer, m_pMainCamera);

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
	m_pAerialLUT->SetShadow(m_pSun->GetShadowMapPtr());

	m_pSun->SetAtmosphere(m_pAtmosphereConstantBuffer);
	m_pSun->SetTransmittanceLUT(m_pTransmittanceLUT->GetTransmittanceLUT());
	m_pSun->Update();
}

void Scene::Update(const float DELTA_TIME)
{
	updateLights(DELTA_TIME);
	updateGlobalConstants(DELTA_TIME);

	const Vector3 CAMERA_POS = m_pMainCamera->GetEyePos();
	const Matrix CAMERA_VIEWPROJECTION = m_pMainCamera->GetView() * m_pMainCamera->GetProjection();
	const float ATMOS_EYE_HEIGHT = m_pAerialLUT->pAerialData->WorldScale * CAMERA_POS.y;
	const FrustumDirection CAMERA_FRUSTUM = m_pMainCamera->GetFrustumDirection();

	// Update Sun predata.
	// m_pSun->SunAngle.x = Clamp(sin(m_pSun->SunAngle.x + DELTA_TIME), 0.0f, 360.0f);
	m_pSun->SunAngle.y = 45.0f;
	m_pSun->SetCamera(&CAMERA_POS, &CAMERA_VIEWPROJECTION);
	m_pSun->SetWorldScale(m_pAerialLUT->pAerialData->WorldScale);
	m_pSun->Update();

	// Update shadow map.
	/*m_SunProperty.Position = m_pSun->SunWorld.Translation();
	m_ShadowMap.Update(m_SunProperty, m_SunCamera, m_Camera);*/

	// Update aerial LUT.
	m_pAerialLUT->SetCamera(&CAMERA_POS, ATMOS_EYE_HEIGHT, &CAMERA_FRUSTUM);
	m_pAerialLUT->SetSun(&m_pSun->SunProperty.Direction);
	m_pAerialLUT->Update();

	// Update sky view LUT.
	m_pSkyLUT->SetCamera(&(CAMERA_POS * m_pAerialLUT->pAerialData->WorldScale));
	m_pSkyLUT->SetSun(&m_pSun->SunProperty.Direction, &(m_pSun->SunIntensity * m_pSun->SunColor));
	m_pSkyLUT->Update();

	// Update Sky.
	m_pSky->SetCamera(&CAMERA_FRUSTUM);
	m_pSky->Update();

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
	if (m_pSkybox)
	{
		delete m_pSkybox;
		m_pSkybox = nullptr;
	}

	SAFE_RELEASE(m_pBrdfSRV);
	SAFE_RELEASE(m_pSpecularSRV);
	SAFE_RELEASE(m_pIrradianceSRV);
	SAFE_RELEASE(m_pEnvSRV);

	Lights.clear();
	for (UINT64 i = 0, size = RenderObjects.size(); i < size; ++i)
	{
		delete RenderObjects[i];
		RenderObjects[i] = nullptr;
	}
	RenderObjects.clear();

	m_pMainCamera = nullptr;
	m_pRenderer = nullptr;
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
	pResourceManager->CreateTextureCubeFromFile((basePath + envFileName).c_str(), m_pEnv->GetTexture2DPPtr(), &textureDesc);*/

	hr = pResourceManager->CreateTextureCubeFromFile((basePath + specularFileName).c_str(), &pTexture, &textureDesc);
	BREAK_IF_FAILED(hr);
	hr = pDevice->CreateShaderResourceView(pTexture, nullptr, &m_pSpecularSRV);
	BREAK_IF_FAILED(hr);
	pTexture->Release();
	pTexture = nullptr;
	/*m_pSpecular = new Texture;
	pResourceManager->CreateTextureCubeFromFile((basePath + specularFileName).c_str(), m_pSpecular->GetTexture2DPPtr(), &textureDesc);*/

	hr = pResourceManager->CreateTextureCubeFromFile((basePath + irradianceFileName).c_str(), &pTexture, &textureDesc);
	BREAK_IF_FAILED(hr);
	hr = pDevice->CreateShaderResourceView(pTexture, nullptr, &m_pIrradianceSRV);
	BREAK_IF_FAILED(hr);
	pTexture->Release();
	pTexture = nullptr;
	/*m_pIrradiance = new Texture;
	pResourceManager->CreateTextureCubeFromFile((basePath + irradianceFileName).c_str(), m_pIrradiance->GetTexture2DPPtr(), &textureDesc);*/

	hr = pResourceManager->CreateTextureCubeFromFile((basePath + brdfFileName).c_str(), &pTexture, &textureDesc);
	BREAK_IF_FAILED(hr);
	hr = pDevice->CreateShaderResourceView(pTexture, nullptr, &m_pBrdfSRV);
	BREAK_IF_FAILED(hr);
	pTexture->Release();
	pTexture = nullptr;
	/*m_pBRDF = new Texture;
	pResourceManager->CreateTextureCubeFromFile((basePath + brdfFileName).c_str(), m_pBRDF->GetTexture2DPPtr(), &textureDesc);*/
}

void Scene::updateLights(const float DELTA_TIME)
{
	for (UINT64 i = 0, size = Lights.size(); i < size; ++i)
	{
		Lights[i].Update(DELTA_TIME, m_pMainCamera);
		m_ppLightSpheres[i]->UpdateWorld(Matrix::CreateScale(Max(0.01f, Lights[i].Property.Radius)) * Matrix::CreateTranslation(Lights[i].Property.Position));
	}
}

void Scene::updateGlobalConstants(const float DELTA_TIME)
{
	const Vector3 EYE_WORLD = m_pMainCamera->GetEyePos();
	const Matrix REFLECTION = Matrix::CreateReflection(m_MirrorPlane);
	const Matrix VIEW = m_pMainCamera->GetView();
	const Matrix PROJECTION = m_pMainCamera->GetProjection();

	GlobalConstants* pGlobalConstData = (GlobalConstants*)m_GlobalConstants.pSystemMem;
	GlobalConstants* pReflectionConstData = (GlobalConstants*)m_ReflectionGlobalConstants.pSystemMem;
	if (!pGlobalConstData || !pReflectionConstData)
	{
		__debugbreak();
	}

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
