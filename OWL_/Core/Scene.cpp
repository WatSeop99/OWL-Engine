#include "../Common.h"
#include "../Geometry/GeometryGenerator.h"
#include "Scene.h"

namespace Core
{
	void Scene::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	{
		Destroy();
		LoadScene(pDevice);

		createBuffers(pDevice);
		m_GBuffer.bIsEnabled = false;
		m_GBuffer.Initialize(pDevice);

		pLights.resize(MAX_LIGHTS);
		m_ppLightSpheres.resize(MAX_LIGHTS);

		// 조명 설정.
		{
			// 조명 0.
			pLights[0].Property.Radiance = Vector3(3.0f);
			pLights[0].Property.FallOffEnd = 10.0f;
			pLights[0].Property.Position = Vector3(0.0f, 0.0f, 0.0f);
			pLights[0].Property.Direction = Vector3(0.0f, 0.0f, 1.0f);
			pLights[0].Property.SpotPower = 3.0f;
			pLights[0].Property.LightType = LIGHT_POINT | LIGHT_SHADOW;
			pLights[0].Property.Radius = 0.04f;
			// pLights[0].Property.LightType = LIGHT_OFF;

			// 조명 1.
			pLights[1].Property.Radiance = Vector3(3.0f);
			pLights[1].Property.FallOffEnd = 10.0f;
			pLights[1].Property.Position = Vector3(1.0f, 1.1f, 2.0f);
			pLights[1].Property.SpotPower = 2.0f;
			pLights[1].Property.Direction = Vector3(0.0f, -0.5f, 1.7f) - pLights[1].Property.Position;
			pLights[1].Property.Direction.Normalize();
			pLights[1].Property.LightType = LIGHT_SPOT | LIGHT_SHADOW;
			pLights[1].Property.Radius = 0.02f;
			// pLights[1].Property.LightType = LIGHT_OFF;

			// 조명 2.
			pLights[2].Property.Radiance = Vector3(4.0f);
			pLights[2].Property.Position = Vector3(5.0f, 5.0f, 5.0f);
			pLights[2].Property.Direction = Vector3(-1.0f, -1.0f, -1.0f);
			pLights[2].Property.Direction.Normalize();
			pLights[2].Property.LightType = LIGHT_DIRECTIONAL | LIGHT_SHADOW;
			pLights[2].Property.Radius = 0.05f;
			// pLights[2].Property.LightType = LIGHT_OFF;
		}

		// 조명 위치 표시.
		{
			for (int i = 0; i < MAX_LIGHTS; ++i)
			{
				Geometry::MeshInfo sphere = INIT_MESH_INFO;
				Geometry::MakeSphere(&sphere, 1.0f, 20, 20);

				m_ppLightSpheres[i] = New Geometry::Model(pDevice, pContext, { sphere });
				m_ppLightSpheres[i]->UpdateWorld(Matrix::CreateTranslation(pLights[i].Property.Position));
				m_ppLightSpheres[i]->pMeshes[0]->MaterialConstants.CPU.AlbedoFactor = Vector3(0.0f);
				m_ppLightSpheres[i]->pMeshes[0]->MaterialConstants.CPU.EmissionFactor = Vector3(1.0f, 1.0f, 0.0f);
				m_ppLightSpheres[i]->bCastShadow = false; // 조명 표시 물체들은 그림자 X.
				for (size_t j = 0, size = m_ppLightSpheres[i]->pMeshes.size(); j < size; ++j)
				{
					Geometry::Mesh* pCurMesh = m_ppLightSpheres[i]->pMeshes[j];
					pCurMesh->MaterialConstants.CPU.AlbedoFactor = Vector3(0.0f);
					pCurMesh->MaterialConstants.CPU.EmissionFactor = Vector3(1.0f, 1.0f, 0.0f);
				}

				m_ppLightSpheres[i]->bIsVisible = true;
				m_ppLightSpheres[i]->Name = "LightSphere" + std::to_string(i);
				m_ppLightSpheres[i]->bIsPickable = false;

				pRenderObjects.push_back(m_ppLightSpheres[i]); // 리스트에 등록.
			}
			// m_ppLightSpheres[0]->bIsVisible = false;
			// m_ppLightSpheres[1]->bIsVisible = false;
			// m_ppLightSpheres[2]->bIsVisible = false;
		}

		// 바닥(거울).
		{
			// https://freepbr.com/materials/stringy-marble-pbr/
			Geometry::MeshInfo mesh = INIT_MESH_INFO;
			Geometry::MakeSquare(&mesh, 10.0f);

			std::wstring path = L"./Assets/Textures/PBR/stringy-marble-ue/";
			mesh.szAlbedoTextureFileName = path + L"stringy_marble_albedo.png";
			mesh.szEmissiveTextureFileName = L"";
			mesh.szAOTextureFileName = path + L"stringy_marble_ao.png";
			mesh.szMetallicTextureFileName = path + L"stringy_marble_Metallic.png";
			mesh.szNormalTextureFileName = path + L"stringy_marble_Normal-dx.png";
			mesh.szRoughnessTextureFileName = path + L"stringy_marble_Roughness.png";

			m_pGround = New Geometry::Model(pDevice, pContext, { mesh });
			m_pGround->pMeshes[0]->MaterialConstants.CPU.AlbedoFactor = Vector3(0.7f);
			m_pGround->pMeshes[0]->MaterialConstants.CPU.EmissionFactor = Vector3(0.0f);
			m_pGround->pMeshes[0]->MaterialConstants.CPU.MetallicFactor = 0.5f;
			m_pGround->pMeshes[0]->MaterialConstants.CPU.RoughnessFactor = 0.3f;

			// Vector3 position = Vector3(0.0f, -1.0f, 0.0f);
			Vector3 position = Vector3(0.0f, -0.5f, 0.0f);
			m_pGround->UpdateWorld(Matrix::CreateRotationX(DirectX::XM_PI * 0.5f) * Matrix::CreateTranslation(position));
			m_pGround->bCastShadow = false; // 바닥은 그림자 만들기 생략.

			m_MirrorPlane = DirectX::SimpleMath::Plane(position, Vector3(0.0f, 1.0f, 0.0f));
			m_pMirror = m_pGround; // 바닥에 거울처럼 반사 구현.
		}
 
		m_GlobalConstants.CPU.StrengthIBL = 0.1f;
		// m_GlobalConstants.CPU.StrengthIBL = 1.0f;

		// 광원마다 shadow map 설정.
		for (size_t i = 0, size = pLights.size(); i < size; ++i)
		{
			// pLights[i].SetShadowSize(1280, 1280); // 사이즈 변경 가능.
			pLights[i].Initialize(pDevice);
		}

		// 환경 박스 초기화.
		Geometry::MeshInfo skyboxMeshInfo = INIT_MESH_INFO;
		Geometry::MakeBox(&skyboxMeshInfo, 40.0f);

		std::reverse(skyboxMeshInfo.Indices.begin(), skyboxMeshInfo.Indices.end());
		m_pSkybox = New Geometry::Model(pDevice, pContext, { skyboxMeshInfo });
		m_pSkybox->Name = "SkyBox";

		// deferred rendering을 위한 스크린 공간 생성.
		Geometry::MeshInfo meshInfo = INIT_MESH_INFO;
		Geometry::MakeSquare(&meshInfo);

		m_pScreenMesh = (Geometry::Mesh*)Malloc(sizeof(Geometry::Mesh));
		*m_pScreenMesh = INIT_MESH;

		HRESULT hr = Graphics::CreateVertexBuffer(pDevice, meshInfo.Vertices, &(m_pScreenMesh->pVertexBuffer));
		BREAK_IF_FAILED(hr);

		m_pScreenMesh->IndexCount = (UINT)(meshInfo.Indices.size());
		hr = Graphics::CreateIndexBuffer(pDevice, meshInfo.Indices, &(m_pScreenMesh->pIndexBuffer));
		BREAK_IF_FAILED(hr);
	}

	void Scene::Update(ID3D11DeviceContext* pContext, const float DELTA_TIME)
	{
		updateLights(pContext, DELTA_TIME);
		updateGlobalConstants(pContext, DELTA_TIME);

		if (m_pMirror != nullptr)
		{
			m_pMirror->UpdateConstantBuffers(pContext);
		}

		for (size_t i = 0, size = pRenderObjects.size(); i < size; ++i)
		{
			Geometry::Model* pCurModel = pRenderObjects[i];
			pCurModel->UpdateConstantBuffers(pContext);
		}
	}

	void Scene::Render(ID3D11DeviceContext* pContext)
	{
		setScreenViewport(pContext);

		// 공통으로 사용하는 샘플러들 설정.
		pContext->VSSetSamplers(0, (UINT)(Graphics::g_ppSamplerStates.size()), Graphics::g_ppSamplerStates.data());
		pContext->PSSetSamplers(0, (UINT)(Graphics::g_ppSamplerStates.size()), Graphics::g_ppSamplerStates.data());

		// 공통으로 사용할 텍스춰들: "Common.hlsli"에서 register(t10)부터 시작
		ID3D11ShaderResourceView* ppCommonSRVs[] = { m_pEnvSRV, m_pSpecularSRV, m_pIrradianceSRV, m_pBrdfSRV };
		UINT numCommonSRVs = _countof(ppCommonSRVs);
		pContext->PSSetShaderResources(10, numCommonSRVs, ppCommonSRVs);

		renderDepthOnly(pContext);
		renderShadowMaps(pContext);

		if (m_GBuffer.bIsEnabled)
		{
			renderGBuffer(pContext);
			renderDeferredLighting(pContext);
		}
		else
		{
			renderOpaqueObjects(pContext);
		}

		renderOptions(pContext);
		renderMirror(pContext);
	}

	void Scene::Destroy()
	{
		// SaveScene();

		m_pMirror = nullptr;
		m_ppLightSpheres.clear();
		if (m_pGround != nullptr)
		{
			delete m_pGround;
			m_pGround = nullptr;
		}
		if (m_pSkybox != nullptr)
		{
			delete m_pSkybox;
			m_pSkybox = nullptr;
		}

		SAFE_RELEASE(m_pBrdfSRV);
		SAFE_RELEASE(m_pSpecularSRV);
		SAFE_RELEASE(m_pIrradianceSRV);
		SAFE_RELEASE(m_pEnvSRV);

		SAFE_RELEASE(m_pDefaultDSV);

		if (m_pScreenMesh != nullptr)
		{
			ReleaseMesh(&m_pScreenMesh);
		}

		pLights.clear();
		for (size_t i = 0, size = pRenderObjects.size(); i < size; ++i)
		{
			delete pRenderObjects[i];
			pRenderObjects[i] = nullptr;
		}
		pRenderObjects.clear();
	}

	void Scene::ResetBuffers(ID3D11Device* pDevice, const bool bUSE_MSAA, const UINT NUM_QUALITY_LEVELS)
	{
		SAFE_RELEASE(m_pDefaultDSV);
		m_DepthOnlyBuffer.Destroy();
		
		if (m_GBuffer.bIsEnabled)
		{
			m_GBuffer.Destroy();
			m_GBuffer.SetScreenWidth(m_ScreenWidth);
			m_GBuffer.SetScreenHeight(m_ScreenHeight);
			m_GBuffer.Initialize(pDevice);
		}

		createDepthBuffers(pDevice, bUSE_MSAA, NUM_QUALITY_LEVELS);
	}

	void Scene::initCubemaps(ID3D11Device* pDevice, std::wstring&& basePath, std::wstring&& envFileName, std::wstring&& specularFileName, std::wstring&& irradianceFileName, std::wstring&& brdfFileName)
	{
		HRESULT hr = S_OK;

		// BRDF LookUp Table은 CubeMap이 아니라 2D 텍스쳐임.
		hr = Graphics::CreateDDSTexture(pDevice, (basePath + envFileName).c_str(), true, &m_pEnvSRV);
		BREAK_IF_FAILED(hr);

		hr = Graphics::CreateDDSTexture(pDevice, (basePath + specularFileName).c_str(), true, &m_pSpecularSRV);
		BREAK_IF_FAILED(hr);

		hr = Graphics::CreateDDSTexture(pDevice, (basePath + irradianceFileName).c_str(), true, &m_pIrradianceSRV);
		BREAK_IF_FAILED(hr);

		hr = Graphics::CreateDDSTexture(pDevice, (basePath + brdfFileName).c_str(), false, &m_pBrdfSRV);
		BREAK_IF_FAILED(hr);
	}

	void Scene::createBuffers(ID3D11Device* pDevice, const bool bUSE_MSAA, const UINT NUM_QUALITY_LEVELS)
	{
		HRESULT hr = S_OK;

		// constants buffers.
		hr = Graphics::CreateConstBuffer(pDevice, m_GlobalConstants.CPU, &(m_GlobalConstants.pGPU));
		BREAK_IF_FAILED(hr);

		hr = Graphics::CreateConstBuffer(pDevice, m_ReflectionGlobalConstants.CPU, &(m_ReflectionGlobalConstants.pGPU));
		BREAK_IF_FAILED(hr);

		hr = Graphics::CreateConstBuffer(pDevice, m_LightConstants, &(m_LightConstants.pGPU));
		BREAK_IF_FAILED(hr);

		createDepthBuffers(pDevice, bUSE_MSAA, NUM_QUALITY_LEVELS);
	}

	void Scene::createDepthBuffers(ID3D11Device* pDevice, const bool bUSE_MSAA, const UINT NUM_QUALITY_LEVELS)
	{
		HRESULT hr = S_OK;

		// depth buffers.
		D3D11_TEXTURE2D_DESC desc = { 0, };
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

		hr = pDevice->CreateDepthStencilView(pDepthStencilBuffer, nullptr, &m_pDefaultDSV);
		RELEASE(pDepthStencilBuffer);
		BREAK_IF_FAILED(hr);

		// Depth 전용.
		desc.Format = DXGI_FORMAT_D32_FLOAT; // DSV 기준 포맷.
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		m_DepthOnlyBuffer.Initialize(pDevice, desc);
	}

	void Scene::updateLights(ID3D11DeviceContext* pContext, const float DELTA_TIME)
	{
		for (size_t i = 0, size = pLights.size(); i < size; ++i)
		{
			pLights[i].Update(pContext, DELTA_TIME, m_Camera);
			m_ppLightSpheres[i]->UpdateWorld(Matrix::CreateScale(std::max(0.01f, pLights[i].Property.Radius)) * Matrix::CreateTranslation(pLights[i].Property.Position));
			memcpy(&m_LightConstants.CPU.Lights[i], &pLights[i].Property, sizeof(LightProperty));
		}

		m_LightConstants.Upload(pContext);
	}

	void Scene::updateGlobalConstants(ID3D11DeviceContext* pContext, const float DELTA_TIME)
	{
		const Vector3 EYE_WORLD = m_Camera.GetEyePos();
		const Matrix REFLECTION = Matrix::CreateReflection(m_MirrorPlane);
		const Matrix VIEW = m_Camera.GetView();
		const Matrix PROJECTION = m_Camera.GetProjection();

		m_GlobalConstants.CPU.GlobalTime += DELTA_TIME;
		m_GlobalConstants.CPU.EyeWorld = EYE_WORLD;
		m_GlobalConstants.CPU.View = VIEW.Transpose();
		m_GlobalConstants.CPU.Projection = PROJECTION.Transpose();
		m_GlobalConstants.CPU.InverseProjection = PROJECTION.Invert().Transpose();
		m_GlobalConstants.CPU.ViewProjection = (VIEW * PROJECTION).Transpose();
		m_GlobalConstants.CPU.InverseView = VIEW.Invert().Transpose();

		// 그림자 렌더링에 사용.
		m_GlobalConstants.CPU.InverseViewProjection = m_GlobalConstants.CPU.ViewProjection.Invert();

		memcpy(&m_ReflectionGlobalConstants.CPU, &m_GlobalConstants.CPU, sizeof(GlobalConstants));
		m_ReflectionGlobalConstants.CPU.View = (REFLECTION * VIEW).Transpose();
		m_ReflectionGlobalConstants.CPU.ViewProjection = (REFLECTION * VIEW * PROJECTION).Transpose();
		// 그림자 렌더링에 사용 (광원의 위치도 반사시킨 후에 계산해야 함).
		m_ReflectionGlobalConstants.CPU.InverseViewProjection = m_ReflectionGlobalConstants.CPU.ViewProjection.Invert();

		m_GlobalConstants.Upload(pContext);
		m_ReflectionGlobalConstants.Upload(pContext);
	}

	void Scene::renderDepthOnly(ID3D11DeviceContext* pContext)
	{
		pContext->OMSetRenderTargets(0, nullptr, m_DepthOnlyBuffer.pDSV);
		pContext->ClearDepthStencilView(m_DepthOnlyBuffer.pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

		setGlobalConstants(pContext, &(m_GlobalConstants.pGPU), 0);
		setPipelineState(pContext, Graphics::g_DepthOnlyPSO);

		for (size_t i = 0, size = pRenderObjects.size(); i < size; ++i)
		{
			Geometry::Model* pCurModel = pRenderObjects[i];
			pCurModel->Render(pContext);
		}
		
		if (m_pSkybox)
		{
			m_pSkybox->Render(pContext);
		}
		if (m_pMirror)
		{
			m_pMirror->Render(pContext);
		}
	}

	void Scene::renderShadowMaps(ID3D11DeviceContext* pContext)
	{
		// 쉐도우 맵을 다른 쉐이더에서 SRV 해제.
		ID3D11ShaderResourceView* ppNulls[2] = { nullptr, };
		pContext->PSSetShaderResources(15, 2, ppNulls);

		for (size_t i = 0, size = pLights.size(); i < size; ++i)
		{
			pLights[i].RenderShadowMap(pContext, pRenderObjects, m_pMirror);
		}
	}

	void Scene::renderOpaqueObjects(ID3D11DeviceContext* pContext)
	{
		// 다시 렌더링 해상도로 되돌리기.
		setScreenViewport(pContext);

		// 거울은 빼고 원래 대로 그리기.
		const float CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		pContext->ClearRenderTargetView(m_FloatBuffer.pRTV, CLEAR_COLOR);
		pContext->ClearDepthStencilView(m_pDefaultDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		pContext->OMSetRenderTargets(1, &(m_FloatBuffer.pRTV), m_pDefaultDSV);

		// 그림자맵들도 공용 텍스춰들 이후에 추가.
		// 주의: 마지막 shadowDSV를 RenderTarget에서 해제한 후 설정.
		ID3D11ShaderResourceView* ppShadowSRVs[MAX_LIGHTS] = { nullptr, };
		UINT pointLightIndex = -1;
		UINT directionalLightIndex = -1;
		for (size_t i = 0, size = pLights.size(); i < size; ++i)
		{
			ShadowMap& curShadowMap = pLights[i].GetShadowMap();

			switch (pLights[i].Property.LightType & (LIGHT_DIRECTIONAL | LIGHT_POINT | LIGHT_SPOT))
			{
			case LIGHT_DIRECTIONAL:
				directionalLightIndex = (UINT)i;
				break;

			case LIGHT_POINT:
				pointLightIndex = (UINT)i;
				break;

			case LIGHT_SPOT:
			{
				Graphics::Texture2D& curShadowBuffer = curShadowMap.GetSpotLightShadowBuffer();
				ppShadowSRVs[i] = curShadowBuffer.pSRV;
			}
				break;

			default:
				break;
			}
		}
		pContext->PSSetShaderResources(15, (UINT)pLights.size(), ppShadowSRVs);

		if (pointLightIndex != -1)
		{
			ShadowMap& curShadowMap = pLights[pointLightIndex].GetShadowMap();
			Graphics::Texture2D& curShadowCubeBuffer = curShadowMap.GetPointLightShadowBuffer();
			pContext->PSSetShaderResources(20, 1, &(curShadowCubeBuffer.pSRV));
		}
		if (directionalLightIndex != -1)
		{
			ShadowMap& curShadowMap = pLights[directionalLightIndex].GetShadowMap();
			Graphics::Texture2D& curShadowCascadeBuffer = curShadowMap.GetDirectionalLightShadowBuffer();
			pContext->PSSetShaderResources(25, 1, &(curShadowCascadeBuffer.pSRV));
		}

		setGlobalConstants(pContext, &(m_GlobalConstants.pGPU), 0);
		setGlobalConstants(pContext, &(m_LightConstants.pGPU), 1);

		// 스카이박스 그리기.
		// 최적화를 하고 싶다면 투명한 물체들만 따로 마지막에 그리면 됨.
		setPipelineState(pContext, bDrawAsWire ? Graphics::g_SkyboxWirePSO : Graphics::g_SkyboxSolidPSO);
		m_pSkybox->Render(pContext);

		for (size_t i = 0, size = pRenderObjects.size(); i < size; ++i)
		{
			Geometry::Model* pCurModel = pRenderObjects[i];
			setPipelineState(pContext, pCurModel->GetPSO(bDrawAsWire));
			pCurModel->Render(pContext);
		}
	}

	void Scene::renderGBuffer(ID3D11DeviceContext* pContext)
	{
		// 다시 렌더링 해상도로 되돌리기.
		setScreenViewport(pContext);

		setGlobalConstants(pContext, &(m_GlobalConstants.pGPU), 0);

		m_GBuffer.PrepareRender(pContext, m_pDefaultDSV);

		for (size_t i = 0, size = pRenderObjects.size(); i < size; ++i)
		{
			Geometry::Model* pCurModel = pRenderObjects[i];
			setPipelineState(pContext, pCurModel->GetGBufferPSO(bDrawAsWire));
			pCurModel->Render(pContext);
		}

		m_GBuffer.AfterRender(pContext);
	}

	void Scene::renderDeferredLighting(ID3D11DeviceContext * pContext)
	{
		// 그림자맵들도 공용 텍스춰들 이후에 추가.
		ID3D11ShaderResourceView* ppShadowSRVs[MAX_LIGHTS] = { nullptr, };
		UINT pointLightIndex = -1;
		UINT directionalLightIndex = -1;
		for (size_t i = 0, size = pLights.size(); i < size; ++i)
		{
			ShadowMap& curShadowMap = pLights[i].GetShadowMap();
			switch (pLights[i].Property.LightType & (LIGHT_DIRECTIONAL | LIGHT_POINT | LIGHT_SPOT))
			{
			case LIGHT_DIRECTIONAL:
				directionalLightIndex = (UINT)i;
				break;

			case LIGHT_POINT:
				pointLightIndex = (UINT)i;
				break;

			case LIGHT_SPOT:
			{
				Graphics::Texture2D& curShadowBuffer = curShadowMap.GetSpotLightShadowBuffer();
				ppShadowSRVs[i] = curShadowBuffer.pSRV;
			}
			break;

			default:
				break;
			}
		}
		pContext->PSSetShaderResources(15, (UINT)pLights.size(), ppShadowSRVs);

		if (pointLightIndex != -1)
		{
			ShadowMap& curShadowMap = pLights[pointLightIndex].GetShadowMap();
			Graphics::Texture2D& curShadowCubeBuffer = curShadowMap.GetPointLightShadowBuffer();
			pContext->PSSetShaderResources(20, 1, &(curShadowCubeBuffer.pSRV));
		}
		if (directionalLightIndex != -1)
		{
			ShadowMap& curShadowMap = pLights[directionalLightIndex].GetShadowMap();
			Graphics::Texture2D& curShadowCascadeBuffer = curShadowMap.GetDirectionalLightShadowBuffer();
			pContext->PSSetShaderResources(25, 1, &(curShadowCascadeBuffer.pSRV));
		}

		setGlobalConstants(pContext, &(m_LightConstants.pGPU), 1);

		const float CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		pContext->ClearRenderTargetView(m_FloatBuffer.pRTV, CLEAR_COLOR);
		pContext->ClearDepthStencilView(m_pDefaultDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		pContext->OMSetRenderTargets(1, &(m_FloatBuffer.pRTV), m_pDefaultDSV);

		ID3D11ShaderResourceView* ppSRVs[5] = { m_GBuffer.AlbedoBuffer.pSRV, m_GBuffer.NormalBuffer.pSRV, m_GBuffer.PositionBuffer.pSRV, m_GBuffer.EmissionBuffer.pSRV, m_GBuffer.ExtraBuffer.pSRV };
		pContext->PSSetShaderResources(0, 5, ppSRVs);

		setPipelineState(pContext, Graphics::g_DeferredRenderingPSO);

		UINT stride = sizeof(Geometry::Vertex);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &(m_pScreenMesh->pVertexBuffer), &stride, &offset);
		pContext->IASetIndexBuffer(m_pScreenMesh->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		pContext->DrawIndexed(m_pScreenMesh->IndexCount, 0, 0);

		pContext->OMSetRenderTargets(1, &(m_FloatBuffer.pRTV), m_GBuffer.DepthBuffer.pDSV);

		// 스카이박스 그리기.
		setPipelineState(pContext, bDrawAsWire ? Graphics::g_SkyboxWirePSO : Graphics::g_SkyboxSolidPSO);
		m_pSkybox->Render(pContext);

	}

	void Scene::renderOptions(ID3D11DeviceContext* pContext)
	{
		for (size_t i = 0, size = pRenderObjects.size(); i < size; ++i)
		{
			Geometry::Model* pCurModel = pRenderObjects[i];
			if (pCurModel->bDrawNormals)
			{
				setPipelineState(pContext, Graphics::g_NormalsPSO);
				pCurModel->RenderNormals(pContext);
			}
			if (bDrawOBB)
			{
				setPipelineState(pContext, Graphics::g_BoundingBoxPSO);
				pCurModel->RenderWireBoundingBox(pContext);
			}
			if (bDrawBS)
			{
				setPipelineState(pContext, Graphics::g_BoundingBoxPSO);
				pCurModel->RenderWireBoundingSphere(pContext);
			}
		}
	}

	void Scene::renderMirror(ID3D11DeviceContext* pContext)
	{
		if (m_pMirror == nullptr)
		{
			return;
		}

		if (MirrorAlpha == 1.0f) // 불투명하면 거울만 그림.
		{
			setPipelineState(pContext, bDrawAsWire ? Graphics::g_DefaultWirePSO : Graphics::g_DefaultSolidPSO);
			m_pMirror->Render(pContext);
		}
		else if (MirrorAlpha < 1.0f) // 투명도가 조금이라도 있으면 처리.
		{
			// 거울 위치만 StencilBuffer에 1로 표기.
			setPipelineState(pContext, Graphics::g_StencilMaskPSO);
			m_pMirror->Render(pContext);

			// 거울 위치에 반사된 물체들을 렌더링.
			setGlobalConstants(pContext, &(m_ReflectionGlobalConstants.pGPU), 0);
			pContext->ClearDepthStencilView(m_pDefaultDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

			for (size_t i = 0, size = pRenderObjects.size(); i < size; ++i)
			{
				Geometry::Model* pCurModel = pRenderObjects[i];
				setPipelineState(pContext, pCurModel->GetPSO(bDrawAsWire));
				pCurModel->Render(pContext);
			}

			setPipelineState(pContext, bDrawAsWire ? Graphics::g_ReflectSkyboxWirePSO : Graphics::g_ReflectSkyboxSolidPSO);
			m_pSkybox->Render(pContext);

			// 거울 자체의 재질을 "Blend"로 그림.
			setGlobalConstants(pContext, &(m_GlobalConstants.pGPU), 0);
			setPipelineState(pContext, bDrawAsWire ? Graphics::g_MirrorBlendWirePSO : Graphics::g_MirrorBlendSolidPSO);
			m_pMirror->Render(pContext);
		}
	}

	void Scene::setPipelineState(ID3D11DeviceContext* pContext, const Graphics::GraphicsPSO& PSO)
	{
		pContext->VSSetShader(PSO.pVertexShader, nullptr, 0);
		pContext->PSSetShader(PSO.pPixelShader, nullptr, 0);
		pContext->HSSetShader(PSO.pHullShader, nullptr, 0);
		pContext->DSSetShader(PSO.pDomainShader, nullptr, 0);
		pContext->GSSetShader(PSO.pGeometryShader, nullptr, 0);
		pContext->CSSetShader(nullptr, nullptr, 0);
		pContext->IASetInputLayout(PSO.pInputLayout);
		pContext->RSSetState(PSO.pRasterizerState);
		pContext->OMSetBlendState(PSO.pBlendState, PSO.BlendFactor, 0xffffffff);
		pContext->OMSetDepthStencilState(PSO.pDepthStencilState, PSO.StencilRef);
		pContext->IASetPrimitiveTopology(PSO.PrimitiveTopology);
	}

	void Scene::setScreenViewport(ID3D11DeviceContext* pContext)
	{
		D3D11_VIEWPORT screenViewport = { 0, };
		screenViewport.TopLeftX = 0;
		screenViewport.TopLeftY = 0;
		screenViewport.Width = (float)m_ScreenWidth;
		screenViewport.Height = (float)m_ScreenHeight;
		screenViewport.MinDepth = 0.0f;
		screenViewport.MaxDepth = 1.0f;

		pContext->RSSetViewports(1, &screenViewport);
	}

	void Scene::setGlobalConstants(ID3D11DeviceContext* pContext, ID3D11Buffer** ppGlobalConstants, UINT slot)
	{ 
		// 쉐이더와 일관성 유지 cbuffer GlobalConstants : register(slot).
		pContext->VSSetConstantBuffers(slot, 1, ppGlobalConstants);
		pContext->PSSetConstantBuffers(slot, 1, ppGlobalConstants);
		pContext->GSSetConstantBuffers(slot, 1, ppGlobalConstants);
	}
}
