#include "../Common.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "../Geometry/GeometryGenerator.h"
#include "BaseRenderer.h"

// imgui_impl_win32.cpp에 정의된 메시지 처리 함수에 대한 전방 선언
// Vcpkg를 통해 IMGUI를 사용할 경우 빨간줄로 경고가 뜰 수 있음
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Core
{
	using namespace DirectX;
	using namespace DirectX::SimpleMath;
	using DirectX::BoundingSphere;
	using DirectX::SimpleMath::Vector3;

	BaseRenderer* g_pAppBase = nullptr;

	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return g_pAppBase->MsgProc(hWnd, msg, wParam, lParam);
	}

	BaseRenderer::BaseRenderer() :
		m_ScreenWidth(1280),
		m_ScreenHeight(720),
		m_hMainWindow(nullptr),
		m_bUseMSAA(true),
		m_NumQualityLevels(0),
		m_bDrawAsWire(false),
		m_bDrawOBB(false),
		m_bDrawBS(false),
		m_BackBufferFormat(DXGI_FORMAT_R8G8B8A8_UNORM),
		m_ShadowWidth(1280),
		m_ShadowHeight(1280),
		m_bLeftButton(false),
		m_bRightButton(false),
		m_bDragStartFlag(false),
		m_MouseNDCX(0.0f),
		m_MouseNDCY(0.0f),
		m_WheelDelta(0.0f),
		m_MouseX(-1),
		m_MouseY(-1),
		m_bLightRotate(false),
		m_bPauseAnimation(false),
		m_MirrorAlpha(1.0f)
	{
		g_pAppBase = this;
		m_Camera.SetAspectRatio(GetAspectRatio());

		initMainWindow();
		initDirect3D();

		// Timer setting.
		m_pTimer = New Timer(m_pDevice5);
	}

	BaseRenderer::~BaseRenderer()
	{
		g_pAppBase = nullptr;

		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		m_pContext4->OMSetRenderTargets(0, nullptr, nullptr);
		m_pContext4->Flush();

		Graphics::DestroyCommonStates();

		if (m_pTimer)
		{
			delete m_pTimer;
			m_pTimer = nullptr;
		}

		for (size_t i = 0, size = m_pBasicList.size(); i < size; ++i)
		{
			delete m_pBasicList[i];
			m_pBasicList[i] = nullptr;
		}
		m_pBasicList.clear();

		m_pMirror = nullptr;
		m_pCursorSphere = nullptr;
		for (int i = 0; i < MAX_LIGHTS; ++i)
		{
			m_ppLightSpheres[i] = nullptr;
		}
		m_pPickedModel = nullptr;
		if (m_pSkybox)
		{
			delete m_pSkybox;
			m_pSkybox = nullptr;
		}
		if (m_pScreenSquare)
		{
			delete m_pScreenSquare;
			m_pScreenSquare = nullptr;
		}

		SAFE_RELEASE(m_pBrdfSRV);
		SAFE_RELEASE(m_pSpecularSRV);
		SAFE_RELEASE(m_pIrradianceSRV);
		SAFE_RELEASE(m_pEnvSRV);

		for (int i = 0; i < MAX_LIGHTS; ++i)
		{
			SAFE_RELEASE(m_ppShadowGlobalConstsGPUs[i]);
			SAFE_RELEASE(m_ppShadowSRVs[i]);
			SAFE_RELEASE(m_ppShadowDSVs[i]);
			SAFE_RELEASE(m_ppShadowBuffers[i]);
		}
		SAFE_RELEASE(m_pReflectGlobalConstsGPU);
		SAFE_RELEASE(m_pGlobalConstsGPU);

		SAFE_RELEASE(m_pDepthOnlySRV);
		SAFE_RELEASE(m_pDefaultDSV);
		SAFE_RELEASE(m_pDepthOnlyDSV);
		SAFE_RELEASE(m_pDepthOnlyBuffer);

		SAFE_RELEASE(m_pPrevSRV);
		SAFE_RELEASE(m_pResolvedSRV);
		SAFE_RELEASE(m_pPrevRTV);
		SAFE_RELEASE(m_pResolvedRTV);
		SAFE_RELEASE(m_pFloatRTV);
		SAFE_RELEASE(m_pPrevBuffer);
		SAFE_RELEASE(m_pResolvedBuffer);
		SAFE_RELEASE(m_pFloatBuffer);

		SAFE_RELEASE(m_pBackBufferRTV);
		SAFE_RELEASE(m_pBackBuffer);
		SAFE_RELEASE(m_pSwapChain);
		SAFE_RELEASE(m_pSwapChain4);
		SAFE_RELEASE(m_pContext);
		SAFE_RELEASE(m_pContext4);
		SAFE_RELEASE(m_pDevice);
		SAFE_RELEASE(m_pDevice5);

		DestroyWindow(m_hMainWindow);
		// UnregisterClass(wc.lpszClassName, wc.hInstance); //생략
	}

	int BaseRenderer::Run()
	{
		// 메인 루프.
		MSG msg = { 0, };
		while (msg.message != WM_QUIT)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				Update(ImGui::GetIO().DeltaTime);
				Render(); // <- 중요: 우리가 구현한 렌더링.

				// GUI 렌더링 후에 Present() 호출.
				m_pSwapChain->Present(1, 0);
			}
		}

		return (int)(msg.wParam);
	}

	void BaseRenderer::Initialize()
	{
		/*initMainWindow();
		initDirect3D();*/
		initGUI();

		m_pTimer->Start(m_pContext4, false);

		InitScene();

		// 환경 박스 초기화.
		struct Geometry::MeshData skyboxMesh = INIT_MESH_DATA;
		Geometry::MakeBox(&skyboxMesh, 40.0f);

		std::reverse(skyboxMesh.Indices.begin(), skyboxMesh.Indices.end());
		m_pSkybox = New Geometry::Model(m_pDevice5, m_pContext4, { skyboxMesh });
		m_pSkybox->Name = "SkyBox";

		// 콘솔창이 렌더링 창을 덮는 것을 방지.
		SetForegroundWindow(m_hMainWindow);

		OutputDebugStringA("Renderer intialize time ==> ");
		m_pTimer->End(m_pContext4);
	}

	// 여러 예제들이 공통적으로 사용하기 좋은 장면 설정
	void BaseRenderer::InitScene()
	{
		// 조명 설정.
		{
			// 조명 0은 고정.
			m_GlobalConstsCPU.Lights[0].Radiance = Vector3(5.0f);
			m_GlobalConstsCPU.Lights[0].Position = Vector3(0.0f, 1.5f, 1.1f);
			m_GlobalConstsCPU.Lights[0].Direction = Vector3(0.0f, -1.0f, 0.0f);
			m_GlobalConstsCPU.Lights[0].SpotPower = 3.0f;
			m_GlobalConstsCPU.Lights[0].Radius = 0.04f;
			m_GlobalConstsCPU.Lights[0].Type = LIGHT_SPOT | LIGHT_SHADOW; // Point with shadow.

			// 조명 1의 위치와 방향은 Update()에서 설정.
			m_GlobalConstsCPU.Lights[1].Radiance = Vector3(5.0f);
			m_GlobalConstsCPU.Lights[1].SpotPower = 3.0f;
			m_GlobalConstsCPU.Lights[1].FallOffEnd = 20.0f;
			m_GlobalConstsCPU.Lights[1].Radius = 0.02f;
			m_GlobalConstsCPU.Lights[1].Type = LIGHT_SPOT | LIGHT_SHADOW; // Point with shadow.

			// 조명 2는 꺼놓음.
			m_GlobalConstsCPU.Lights[2].Type = LIGHT_OFF;
		}

		// 조명 위치 표시.
		{
			for (int i = 0; i < MAX_LIGHTS; ++i)
			{
				struct Geometry::MeshData sphere = INIT_MESH_DATA;
				Geometry::MakeSphere(&sphere, 1.0f, 20, 20);

				m_ppLightSpheres[i] = New Geometry::Model(m_pDevice5, m_pContext4, { sphere });
				m_ppLightSpheres[i]->UpdateWorld(Matrix::CreateTranslation(m_GlobalConstsCPU.Lights[i].Position));
				m_ppLightSpheres[i]->MaterialConstants.CPU.AlbedoFactor = Vector3(0.0f);
				m_ppLightSpheres[i]->MaterialConstants.CPU.EmissionFactor = Vector3(1.0f, 1.0f, 0.0f);
				m_ppLightSpheres[i]->bCastShadow = false; // 조명 표시 물체들은 그림자 X.

				// if (m_GlobalConstsCPU.lights[i].type == 0)
				m_ppLightSpheres[i]->bIsVisible = false;
				m_ppLightSpheres[i]->Name = "LightSphere" + std::to_string(i);
				m_ppLightSpheres[i]->bIsPickable = false;

				m_pBasicList.push_back(m_ppLightSpheres[i]); // 리스트에 등록.
			}
		}

		// 커서 표시 (Main sphere와의 충돌이 감지되면 월드 공간에 작게 그려지는 구).
		{
			struct Geometry::MeshData sphere = INIT_MESH_DATA;
			Geometry::MakeSphere(&sphere, 0.01f, 10, 10);

			m_pCursorSphere = New Geometry::Model(m_pDevice5, m_pContext4, { sphere });
			m_pCursorSphere->bIsVisible = false; // 마우스가 눌렸을 때만 보임
			m_pCursorSphere->bCastShadow = false; // 그림자 X
			m_pCursorSphere->MaterialConstants.CPU.AlbedoFactor = Vector3(0.0f);
			m_pCursorSphere->MaterialConstants.CPU.EmissionFactor = Vector3(0.0f, 1.0f, 0.0f);

			m_pBasicList.push_back(m_pCursorSphere); // 리스트에 등록.
		}
	}

	void BaseRenderer::InitCubemaps(std::wstring&& basePath, std::wstring&& envFileName, std::wstring&& specularFileName, std::wstring&& irradianceFileName, std::wstring&& brdfFileName)
	{
		HRESULT hr = S_OK;

		// BRDF LookUp Table은 CubeMap이 아니라 2D 텍스춰 입니다.
		hr = Graphics::CreateDDSTexture(m_pDevice5, (basePath + envFileName).c_str(), true, &m_pEnvSRV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pEnvSRV, "m_pEnvSRV");

		hr = Graphics::CreateDDSTexture(m_pDevice5, (basePath + specularFileName).c_str(), true, &m_pSpecularSRV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pSpecularSRV, "m_pSpecularSRV");

		hr = Graphics::CreateDDSTexture(m_pDevice5, (basePath + irradianceFileName).c_str(), true, &m_pIrradianceSRV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pIrradianceSRV, "m_pIrradianceSRV");

		hr = Graphics::CreateDDSTexture(m_pDevice5, (basePath + brdfFileName).c_str(), false, &m_pBrdfSRV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pBrdfSRV, "m_pBrdfSRV");
	}

	void BaseRenderer::UpdateGUI()
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();
		ImGui::Begin("Scene Control");

		// ImGui가 측정해주는 Framerate 출력.
		ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}

	void BaseRenderer::UpdateLights(float dt)
	{
		// 회전하는 lights[1] 업데이트.
		static Vector3 lightDev = Vector3(1.0f, 0.0f, 0.0f);
		if (m_bLightRotate)
		{
			lightDev = Vector3::Transform(lightDev, Matrix::CreateRotationY(dt * DirectX::XM_PI * 0.5f));
		}

		Vector3 focusPosition = Vector3(0.0f, -0.5f, 1.7f);
		m_GlobalConstsCPU.Lights[1].Position = Vector3(0.0f, 1.1f, 2.0f) + lightDev;
		m_GlobalConstsCPU.Lights[1].Direction = focusPosition - m_GlobalConstsCPU.Lights[1].Position;
		m_GlobalConstsCPU.Lights[1].Direction.Normalize();

		// 그림자맵을 만들기 위한 시점.
		for (int i = 0; i < MAX_LIGHTS; ++i)
		{
			const struct Light& light = m_GlobalConstsCPU.Lights[i];
			if (light.Type & LIGHT_SHADOW)
			{
				Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
				if (abs(up.Dot(light.Direction) + 1.0f) < 1e-5)
				{
					up = Vector3(1.0f, 0.0f, 0.0f);
				}

				// 그림자맵을 만들 때 필요.
				Matrix lightView = XMMatrixLookAtLH(light.Position, light.Position + light.Direction, up);
				Matrix lightProj = XMMatrixPerspectiveFovLH(XMConvertToRadians(120.0f), 1.0f, 0.1f, 10.0f);

				m_pShadowGlobalConstsCPUs[i].EyeWorld = light.Position;
				m_pShadowGlobalConstsCPUs[i].View = lightView.Transpose();
				m_pShadowGlobalConstsCPUs[i].Projection = lightProj.Transpose();
				m_pShadowGlobalConstsCPUs[i].InverseProjection = lightProj.Invert().Transpose();
				m_pShadowGlobalConstsCPUs[i].ViewProjection = (lightView * lightProj).Transpose();

				Graphics::UpdateBuffer(m_pContext4, m_pShadowGlobalConstsCPUs[i], m_ppShadowGlobalConstsGPUs[i]);

				// 그림자를 실제로 렌더링할 때 필요.
				// 반사된 장면에서도 그림자를 그리고 싶다면 조명도 반사시켜서 넣어주면 됨.
				m_GlobalConstsCPU.Lights[i].ViewProjection = m_pShadowGlobalConstsCPUs[i].ViewProjection;
				m_GlobalConstsCPU.Lights[i].InverseProjection = m_pShadowGlobalConstsCPUs[i].InverseProjection;
			}
		}
	}

	// 여러 물체들이 공통적으료 사용하는 Const 업데이트.
	void BaseRenderer::UpdateGlobalConstants(const float& DELTA_TIME, const Vector3& EYE_WORLD, const Matrix& VIEW, const Matrix& PROJECTION, const Matrix& REFLECTION)
	{
		m_GlobalConstsCPU.GlobalTime += DELTA_TIME;
		m_GlobalConstsCPU.EyeWorld = EYE_WORLD;
		m_GlobalConstsCPU.View = VIEW.Transpose();
		m_GlobalConstsCPU.Projection = PROJECTION.Transpose();
		m_GlobalConstsCPU.InverseProjection = PROJECTION.Invert().Transpose();
		m_GlobalConstsCPU.ViewProjection = (VIEW * PROJECTION).Transpose();
		m_GlobalConstsCPU.InverseView = VIEW.Invert().Transpose();

		// 그림자 렌더링에 사용.
		m_GlobalConstsCPU.InverseViewProjection = m_GlobalConstsCPU.ViewProjection.Invert();

		// m_ReflectGlobalConstsCPU = m_GlobalConstsCPU;
		memcpy(&m_ReflectGlobalConstsCPU, &m_GlobalConstsCPU, sizeof(m_GlobalConstsCPU));
		m_ReflectGlobalConstsCPU.View = (REFLECTION * VIEW).Transpose();
		m_ReflectGlobalConstsCPU.ViewProjection = (REFLECTION * VIEW * PROJECTION).Transpose();
		// 그림자 렌더링에 사용 (광원의 위치도 반사시킨 후에 계산해야 함).
		m_ReflectGlobalConstsCPU.InverseViewProjection = m_ReflectGlobalConstsCPU.ViewProjection.Invert();

		Graphics::UpdateBuffer(m_pContext4, m_GlobalConstsCPU, m_pGlobalConstsGPU);
		Graphics::UpdateBuffer(m_pContext4, m_ReflectGlobalConstsCPU, m_pReflectGlobalConstsGPU);
	}

	void BaseRenderer::Update(float deltaTime)
	{
		UpdateGUI();

		// 카메라의 이동.
		m_Camera.UpdateKeyboard(deltaTime, m_pbKeyPressed);

		// 반사 행렬 추가
		const Vector3 EYE_WORLD = m_Camera.GetEyePos();
		const Matrix REFLECTION = Matrix::CreateReflection(m_MirrorPlane);
		const Matrix VIEW = m_Camera.GetView();
		const Matrix PROJECTION = m_Camera.GetProjection();

		UpdateLights(deltaTime);
		UpdateGlobalConstants(deltaTime, EYE_WORLD, VIEW, PROJECTION, REFLECTION);
		ProcessMouseControl();

		// 거울은 따로 처리.
		if (m_pMirror != nullptr)
		{
			m_pMirror->UpdateConstantBuffers(m_pDevice5, m_pContext4);
		}

		// 조명의 위치 반영.
		for (int i = 0; i < MAX_LIGHTS; ++i)
		{
			m_ppLightSpheres[i]->UpdateWorld(Matrix::CreateScale(std::max(0.01f, m_GlobalConstsCPU.Lights[i].Radius)) *
											 Matrix::CreateTranslation(m_GlobalConstsCPU.Lights[i].Position));
		}

		for (size_t i = 0, size = m_pBasicList.size(); i < size; ++i)
		{
			Geometry::Model* pCurModel = m_pBasicList[i];
			pCurModel->UpdateConstantBuffers(m_pDevice5, m_pContext4);
		}

		m_PostProcessor.Update(m_pContext4);
	}

	void BaseRenderer::RenderDepthOnly()
	{
		m_pContext4->OMSetRenderTargets(0, nullptr, m_pDepthOnlyDSV);
		m_pContext4->ClearDepthStencilView(m_pDepthOnlyDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

		SetGlobalConsts(&m_pGlobalConstsGPU);
		for (size_t i = 0, size = m_pBasicList.size(); i < size; ++i)
		{
			Geometry::Model* pCurModel = m_pBasicList[i];
			SetPipelineState(pCurModel->GetDepthOnlyPSO());
			pCurModel->Render(m_pContext4);
		}

		SetPipelineState(Graphics::g_DepthOnlyPSO);
		if (m_pSkybox)
		{
			m_pSkybox->Render(m_pContext4);
		}
		if (m_pMirror)
		{
			m_pMirror->Render(m_pContext4);
		}
	}

	void BaseRenderer::RenderShadowMaps()
	{
		// 쉐도우 맵을 다른 쉐이더에서 SRV 해제.
		ID3D11ShaderResourceView* ppNulls[2] = { nullptr, };
		m_pContext4->PSSetShaderResources(15, 2, ppNulls);

		setShadowViewport(); // 그림자맵 해상도.
		for (int i = 0; i < MAX_LIGHTS; ++i)
		{
			if (m_GlobalConstsCPU.Lights[i].Type & LIGHT_SHADOW)
			{
				m_pContext4->OMSetRenderTargets(0, nullptr, m_ppShadowDSVs[i]);
				m_pContext4->ClearDepthStencilView(m_ppShadowDSVs[i], D3D11_CLEAR_DEPTH, 1.0f, 0);
				SetGlobalConsts(&m_ppShadowGlobalConstsGPUs[i]);

				for (size_t i = 0, size = m_pBasicList.size(); i < size; ++i)
				{
					Geometry::Model* pCurModel = m_pBasicList[i];
					if (pCurModel->bCastShadow && pCurModel->bIsVisible)
					{
						SetPipelineState(pCurModel->GetDepthOnlyPSO());
						pCurModel->Render(m_pContext4);
					}
				}

				if (m_pMirror && m_pMirror->bCastShadow)
				{
					m_pMirror->Render(m_pContext4);
				}
			}
		}
	}

	void BaseRenderer::RenderOpaqueObjects()
	{
		// 다시 렌더링 해상도로 되돌리기.
		setMainViewport();

		// 거울은 빼고 원래 대로 그리기.
		const float CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_pContext4->ClearRenderTargetView(m_pFloatRTV, CLEAR_COLOR);
		m_pContext4->OMSetRenderTargets(1, &m_pFloatRTV, m_pDefaultDSV);

		// 그림자맵들도 공용 텍스춰들 이후에 추가.
		// 주의: 마지막 shadowDSV를 RenderTarget에서 해제한 후 설정.
		ID3D11ShaderResourceView* ppShadowSRVs[MAX_LIGHTS] = { nullptr, };
		for (int i = 0; i < MAX_LIGHTS; ++i)
		{
			ppShadowSRVs[i] = m_ppShadowSRVs[i];
		}
		m_pContext4->PSSetShaderResources(15, MAX_LIGHTS, ppShadowSRVs);
		m_pContext4->ClearDepthStencilView(m_pDefaultDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		SetGlobalConsts(&m_pGlobalConstsGPU);

		// 스카이박스 그리기.
		// 최적화를 하고 싶다면 투명한 물체들만 따로 마지막에 그리면 됨.
		SetPipelineState(m_bDrawAsWire ? Graphics::g_SkyboxWirePSO : Graphics::g_SkyboxSolidPSO);
		m_pSkybox->Render(m_pContext4);

		for (size_t i = 0, size = m_pBasicList.size(); i < size; ++i)
		{
			Geometry::Model* pCurModel = m_pBasicList[i];
			SetPipelineState(pCurModel->GetPSO(m_bDrawAsWire));
			pCurModel->Render(m_pContext4);
		}

		// 노멀 벡터 그리기.
		SetPipelineState(Graphics::g_NormalsPSO);
		for (size_t i = 0, size = m_pBasicList.size(); i < size; ++i)
		{
			Geometry::Model* pCurModel = m_pBasicList[i];
			if (pCurModel->bDrawNormals)
			{
				pCurModel->RenderNormals(m_pContext4);
			}
		}

		SetPipelineState(Graphics::g_BoundingBoxPSO);
		if (m_bDrawOBB)
		{
			for (auto& model : m_pBasicList)
			{
				model->RenderWireBoundingBox(m_pContext4);
			}
			for (size_t i = 0, size = m_pBasicList.size(); i < size; ++i)
			{
				Geometry::Model* pCurModel = m_pBasicList[i];
				pCurModel->RenderWireBoundingBox(m_pContext4);
			}
		}
		if (m_bDrawBS)
		{
			for (size_t i = 0, size = m_pBasicList.size(); i < size; ++i)
			{
				Geometry::Model* pCurModel = m_pBasicList[i];
				pCurModel->RenderWireBoundingSphere(m_pContext4);
			}
		}
	}

	//void BaseRenderer::RenderGBuffer()
	//{
	//	// Render obeject
	//	

	//	// Render Terrain
	//	// Render Foliage
	//}

	//void BaseRenderer::RenderLights()
	//{

	//}

	void BaseRenderer::RenderMirror()
	{
		if (m_pMirror == nullptr)
		{
			return;
		}

		if (m_MirrorAlpha == 1.0f) // 불투명하면 거울만 그림.
		{
			SetPipelineState(m_bDrawAsWire ? Graphics::g_DefaultWirePSO : Graphics::g_DefaultSolidPSO);
			m_pMirror->Render(m_pContext4);
		}
		else if (m_MirrorAlpha < 1.0f) // 투명도가 조금이라도 있으면 반사 처리.
		{
			// 거울 위치만 StencilBuffer에 1로 표기.
			SetPipelineState(Graphics::g_StencilMaskPSO);
			m_pMirror->Render(m_pContext4);

			// 거울 위치에 반사된 물체들을 렌더링.
			SetGlobalConsts(&m_pReflectGlobalConstsGPU);
			m_pContext4->ClearDepthStencilView(m_pDefaultDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

			for (size_t i = 0, size = m_pBasicList.size(); i < size; ++i)
			{
				Geometry::Model* pCurModel = m_pBasicList[i];
				SetPipelineState(pCurModel->GetPSO(m_bDrawAsWire));
				pCurModel->Render(m_pContext4);
			}

			SetPipelineState(m_bDrawAsWire ? Graphics::g_ReflectSkyboxWirePSO : Graphics::g_ReflectSkyboxSolidPSO);
			m_pSkybox->Render(m_pContext4);

			// 거울 자체의 재질을 "Blend"로 그림.
			SetPipelineState(m_bDrawAsWire ? Graphics::g_MirrorBlendWirePSO : Graphics::g_MirrorBlendSolidPSO);
			SetGlobalConsts(&m_pGlobalConstsGPU);
			m_pMirror->Render(m_pContext4);
		}
	}

	void BaseRenderer::RenderGUI()
	{
		// Example의 Render()에서 RT 설정을 해주지 않았을 경우에도
		// 백 버퍼에 GUI를 그리기위해 RT 설정.
		// 예) Render()에서 ComputeShader만 사용
		setMainViewport();

		// GUI 렌더링.
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	void BaseRenderer::Render()
	{
		setMainViewport();

		// 공통으로 사용하는 샘플러들 설정.
		m_pContext4->VSSetSamplers(0, (UINT)(Graphics::g_ppSamplerStates.size()), Graphics::g_ppSamplerStates.data());
		m_pContext4->PSSetSamplers(0, (UINT)(Graphics::g_ppSamplerStates.size()), Graphics::g_ppSamplerStates.data());

		// 공통으로 사용할 텍스춰들: "Common.hlsli"에서 register(t10)부터 시작
		ID3D11ShaderResourceView* ppCommonSRVs[] = { m_pEnvSRV, m_pSpecularSRV, m_pIrradianceSRV, m_pBrdfSRV };
		UINT numCommonSRVs = _countof(ppCommonSRVs);
		m_pContext4->PSSetShaderResources(10, numCommonSRVs, ppCommonSRVs);

		RenderDepthOnly();
		RenderShadowMaps();
		RenderOpaqueObjects();
		RenderMirror();
		m_PostProcessor.Render(m_pContext4);
		RenderGUI(); // 추후 editor/game 모드를 설정하여 따로 렌더링하도록 구상.

		////////////////////// deferred shading //////////////////
		// Culling();
		//RenderDepthOnly();
		//RenderGBuffer();
		//RenderShadowMaps();
		//RenderLights(); // local illumination.
		//RenderMirror();
		//m_PostProcessor.Render(m_pContext4);
		//RenderGUI();
	}

	void BaseRenderer::OnMouseMove(int mouseX, int mouseY)
	{
		m_MouseX = mouseX;
		m_MouseY = mouseY;

		// 마우스 커서의 위치를 NDC로 변환.
		// 마우스 커서는 좌측 상단 (0, 0), 우측 하단(width-1, height-1).
		// NDC는 좌측 하단이 (-1, -1), 우측 상단(1, 1).
		m_MouseNDCX = mouseX * 2.0f / m_ScreenWidth - 1.0f;
		m_MouseNDCY = -mouseY * 2.0f / m_ScreenHeight + 1.0f;

		// 커서가 화면 밖으로 나갔을 경우 범위 조절.
		// 게임에서는 클램프를 안할 수도 있습니다..
		m_MouseNDCX = std::clamp(m_MouseNDCX, -1.0f, 1.0f);
		m_MouseNDCY = std::clamp(m_MouseNDCY, -1.0f, 1.0f);

		// 카메라 시점 회전.
		m_Camera.UpdateMouse(m_MouseNDCX, m_MouseNDCY);
	}

	void BaseRenderer::OnMouseClick(int mouseX, int mouseY)
	{
		m_MouseX = mouseX;
		m_MouseY = mouseY;

		m_MouseNDCX = mouseX * 2.0f / m_ScreenWidth - 1.0f;
		m_MouseNDCY = -mouseY * 2.0f / m_ScreenHeight + 1.0f;
	}

	LRESULT BaseRenderer::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		{
			return true;
		}

		switch (msg)
		{
		case WM_SIZE:
		{// 화면 해상도가 바뀌면 SwapChain을 다시 생성.
			if (m_pSwapChain4)
			{
				m_ScreenWidth = (int)LOWORD(lParam);
				m_ScreenHeight = (int)HIWORD(lParam);

				// 윈도우가 Minimize 모드에서는 screenWidth/Height가 0.
				if (m_ScreenWidth && m_ScreenHeight)
				{
#ifdef _DEBUG
					char debugString[256] = { 0, };
					OutputDebugStringA("Resize SwapChain to ");
					sprintf(debugString, "%d", m_ScreenWidth);
					OutputDebugStringA(debugString);
					OutputDebugStringA(" ");
					sprintf(debugString, "%d", m_ScreenHeight);
					OutputDebugStringA(debugString);
					OutputDebugStringA("\n");
#endif 

					// 기존 버퍼 초기화.
					destroyBuffersForRendering();
					m_pSwapChain4->ResizeBuffers(0,                  // 현재 개수 유지.
												(UINT)m_ScreenWidth, // 해상도 변경.
												(UINT)m_ScreenHeight,
												DXGI_FORMAT_UNKNOWN, // 현재 포맷 유지.
												0);


					createBuffers();
					setMainViewport();
					m_Camera.SetAspectRatio(GetAspectRatio());
					m_PostProcessor.Initialize(m_pDevice5, m_pContext4,
											   { m_pGlobalConstsGPU, m_pBackBuffer, m_pFloatBuffer, m_pResolvedBuffer, m_pPrevBuffer, m_pBackBufferRTV, m_pResolvedSRV, m_pPrevSRV, m_pDepthOnlySRV },
											   m_ScreenWidth, m_ScreenHeight, 4);
				}
			}

			break;
		}

		case WM_SYSCOMMAND:
		{
			if ((wParam & 0xfff0) == SC_KEYMENU) // ALT키 비활성화.
			{
				return 0;
			}

			break;
		}

		case WM_MOUSEMOVE:
			OnMouseMove(LOWORD(lParam), HIWORD(lParam));
			break;

		case WM_LBUTTONDOWN:
		{
			if (!m_bLeftButton)
			{
				m_bDragStartFlag = true; // 드래그를 새로 시작하는지 확인.
			}
			m_bLeftButton = true;
			OnMouseClick(LOWORD(lParam), HIWORD(lParam));

			break;
		}

		case WM_LBUTTONUP:
			m_bLeftButton = false;
			break;

		case WM_RBUTTONDOWN:
		{
			if (!m_bRightButton)
			{
				m_bDragStartFlag = true; // 드래그를 새로 시작하는지 확인.
			}
			m_bRightButton = true;

			break;
		}

		case WM_RBUTTONUP:
			m_bRightButton = false;
			break;

		case WM_KEYDOWN:
		{
			m_pbKeyPressed[wParam] = true;
			if (wParam == VK_ESCAPE) // ESC키 종료.
			{
				DestroyWindow(hwnd);
			}
			if (wParam == VK_SPACE)
			{
				m_bLightRotate = !m_bLightRotate;
			}

			break;
		}

		case WM_KEYUP:
		{
			if (wParam == 'F')  // f키 일인칭 시점.
			{
				m_Camera.bUseFirstPersonView = !m_Camera.bUseFirstPersonView;
			}
			if (wParam == 'C') // c키 화면 캡쳐.
			{
				ID3D11Texture2D* pBackBuffer = nullptr;
				m_pSwapChain4->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
				Graphics::WriteToPngFile(m_pDevice5, m_pContext4, pBackBuffer, L"captured.png");
				RELEASE(pBackBuffer);
			}
			if (wParam == 'P') // 애니메이션 일시중지할 때 사용.
			{
				m_bPauseAnimation = !m_bPauseAnimation;
			}
			if (wParam == 'Z') // 카메라 설정 화면에 출력.
			{
				m_Camera.PrintView();
			}

			m_pbKeyPressed[wParam] = false;
			break;
		}

		case WM_MOUSEWHEEL:
			m_WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		default:
			break;
		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	void BaseRenderer::SetGlobalConsts(ID3D11Buffer** ppGlobalConstsGPU)
	{
		// 쉐이더와 일관성 유지 cbuffer GlobalConstants : register(b0).
		m_pContext4->VSSetConstantBuffers(0, 1, ppGlobalConstsGPU);
		m_pContext4->PSSetConstantBuffers(0, 1, ppGlobalConstsGPU);
		m_pContext4->GSSetConstantBuffers(0, 1, ppGlobalConstsGPU);
	}

	void BaseRenderer::SetPipelineState(const Graphics::GraphicsPSO& PSO)
	{
		m_pContext4->VSSetShader(PSO.pVertexShader, nullptr, 0);
		m_pContext4->PSSetShader(PSO.pPixelShader, nullptr, 0);
		m_pContext4->HSSetShader(PSO.pHullShader, nullptr, 0);
		m_pContext4->DSSetShader(PSO.pDomainShader, nullptr, 0);
		m_pContext4->GSSetShader(PSO.pGeometryShader, nullptr, 0);
		m_pContext4->CSSetShader(nullptr, nullptr, 0);
		m_pContext4->IASetInputLayout(PSO.pInputLayout);
		m_pContext4->RSSetState(PSO.pRasterizerState);
		m_pContext4->OMSetBlendState(PSO.pBlendState, PSO.BlendFactor, 0xffffffff);
		m_pContext4->OMSetDepthStencilState(PSO.pDepthStencilState, PSO.StencilRef);
		m_pContext4->IASetPrimitiveTopology(PSO.PrimitiveTopology);
	}

	void BaseRenderer::SetPipelineState(const Graphics::ComputePSO& PSO)
	{
		m_pContext4->VSSetShader(nullptr, nullptr, 0);
		m_pContext4->PSSetShader(nullptr, nullptr, 0);
		m_pContext4->HSSetShader(nullptr, nullptr, 0);
		m_pContext4->DSSetShader(nullptr, nullptr, 0);
		m_pContext4->GSSetShader(nullptr, nullptr, 0);
		m_pContext4->CSSetShader(PSO.pComputeShader, nullptr, 0);
	}

	Geometry::Model* BaseRenderer::PickClosest(const Ray& PICKNG_RAY, float* pMinDist)
	{
		*pMinDist = 1e5f;
		Geometry::Model* pMinModel = nullptr;

		for (size_t i = 0, size = m_pBasicList.size(); i < size; ++i)
		{
			Geometry::Model* pCurModel = m_pBasicList[i];
			float dist = 0.0f;
			if (pCurModel->bIsPickable &&
				PICKNG_RAY.Intersects(pCurModel->BoundingSphere, dist) &&
				dist < *pMinDist)
			{
				pMinModel = pCurModel;
				*pMinDist = dist;
			}
		}

		return pMinModel;
	}

	void BaseRenderer::ProcessMouseControl()
	{
		static Geometry::Model* s_pActiveModel = nullptr;
		static float s_PrevRatio = 0.0f;
		static Vector3 s_PrevPos(0.0f);
		static Vector3 s_PrevVector(0.0f);

		// 적용할 회전과 이동 초기화.
		Quaternion q = Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), 0.0f);
		Vector3 dragTranslation(0.0f);
		Vector3 pickPoint(0.0f);
		float dist = 0.0f;

		// 사용자가 두 버튼 중 하나만 누른다고 가정.
		if (m_bLeftButton || m_bRightButton)
		{
			const Matrix VIEW = m_Camera.GetView();
			const Matrix PROJECTION = m_Camera.GetProjection();
			const Vector3 NDC_NEAR = Vector3(m_MouseNDCX, m_MouseNDCY, 0.0f);
			const Vector3 NDC_FAR = Vector3(m_MouseNDCX, m_MouseNDCY, 1.0f);
			const Matrix INV_PROJECTION_VIEW = (VIEW * PROJECTION).Invert();
			const Vector3 WORLD_NEAR = Vector3::Transform(NDC_NEAR, INV_PROJECTION_VIEW);
			const Vector3 WORLD_FAR = Vector3::Transform(NDC_FAR, INV_PROJECTION_VIEW);
			Vector3 dir = WORLD_FAR - WORLD_NEAR;
			dir.Normalize();
			const Ray CUR_RAY = SimpleMath::Ray(WORLD_NEAR, dir);

			if (!s_pActiveModel) // 이전 프레임에서 아무 물체도 선택되지 않았을 경우에는 새로 선택.
			{
				Geometry::Model* pSelectedModel = PickClosest(CUR_RAY, &dist);
				if (pSelectedModel)
				{
					OutputDebugStringA("Newly selected model: ");
					OutputDebugStringA(pSelectedModel->Name.c_str());
					OutputDebugStringA("\n");

					s_pActiveModel = pSelectedModel;
					m_pPickedModel = pSelectedModel; // GUI 조작용 포인터.
					pickPoint = CUR_RAY.position + dist * CUR_RAY.direction;
					if (m_bLeftButton) // 왼쪽 버튼 회전 준비.
					{
						s_PrevVector = pickPoint - s_pActiveModel->BoundingSphere.Center;
						s_PrevVector.Normalize();
					}
					else
					{ // 오른쪽 버튼 이동 준비
						m_bDragStartFlag = false;
						s_PrevRatio = dist / (WORLD_FAR - WORLD_NEAR).Length();
						s_PrevPos = pickPoint;
					}
				}
			}
			else // 이미 선택된 물체가 있었던 경우.
			{
				if (m_bLeftButton) // 왼쪽 버튼으로 계속 회전.
				{
					if (CUR_RAY.Intersects(s_pActiveModel->BoundingSphere, dist))
					{
						pickPoint = CUR_RAY.position + dist * CUR_RAY.direction;
					}
					else // 바운딩 스피어에 가장 가까운 점을 찾기.
					{
						Vector3 c = s_pActiveModel->BoundingSphere.Center - WORLD_NEAR;
						Vector3 centerToRay = dir.Dot(c) * dir - c;
						pickPoint = c + centerToRay * std::clamp(s_pActiveModel->BoundingSphere.Radius / centerToRay.Length(), 0.0f, 1.0f);
						pickPoint += WORLD_NEAR;
					}

					Vector3 currentVector = pickPoint - s_pActiveModel->BoundingSphere.Center;
					currentVector.Normalize();
					float theta = acos(s_PrevVector.Dot(currentVector));
					if (theta > DirectX::XM_PI / 180.0f * 3.0f)
					{
						Vector3 axis = s_PrevVector.Cross(currentVector);
						axis.Normalize();
						q = SimpleMath::Quaternion::CreateFromAxisAngle(axis, theta);
						s_PrevVector = currentVector;
					}

				}
				else // 오른쪽 버튼으로 계속 이동.
				{
					Vector3 newPos = WORLD_NEAR + s_PrevRatio * (WORLD_FAR - WORLD_NEAR);
					if ((newPos - s_PrevPos).Length() > 1e-3)
					{
						dragTranslation = newPos - s_PrevPos;
						s_PrevPos = newPos;
					}
					pickPoint = newPos; // Cursor sphere 그려질 위치.
				}
			}
		}
		else
		{
			// 버튼에서 손을 땠을 경우에는 움직일 모델은 nullptr로 설정.
			s_pActiveModel = nullptr;
		}

		// Cursor sphere 그리기.
		if (s_pActiveModel)
		{
			Vector3 translation = s_pActiveModel->World.Translation();
			s_pActiveModel->World.Translation(Vector3(0.0f));
			s_pActiveModel->UpdateWorld(s_pActiveModel->World * Matrix::CreateFromQuaternion(q) *
										Matrix::CreateTranslation(dragTranslation + translation));
			s_pActiveModel->BoundingSphere.Center = s_pActiveModel->World.Translation();

			// 충돌 지점에 작은 구 그리기.
			m_pCursorSphere->bIsVisible = true;
			m_pCursorSphere->UpdateWorld(Matrix::CreateTranslation(pickPoint));
		}
		else
		{
			m_pCursorSphere->bIsVisible = false;
		}
	}

	void BaseRenderer::initMainWindow()
	{
		WNDCLASSEX wc =
		{
			sizeof(WNDCLASSEX),			// cbSize
			CS_HREDRAW | CS_VREDRAW,	// style
			WndProc,					// lpfnWndProc
			0,							// cbClsExtra
			0,							// cbWndExtra
			GetModuleHandle(NULL),		// hInstance
			NULL, 						// hIcon
			NULL,						// hCursor
			(HBRUSH)(COLOR_WINDOW + 1),	// hbrBackground
			nullptr,					// lpszMenuName
			L"OWL",						// lpszClassName
			NULL						// hIconSm
		};

		if (!RegisterClassEx(&wc))
		{
			__debugbreak();
		}

		RECT wr = { 0, 0, m_ScreenWidth, m_ScreenHeight };
		AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
		m_hMainWindow = CreateWindow(wc.lpszClassName,
									 L"OWL Engine",
									 WS_OVERLAPPEDWINDOW,
									 100,				 // 윈도우 좌측 상단의 x 좌표
									 100,				 // 윈도우 좌측 상단의 y 좌표
									 wr.right - wr.left, // 윈도우 가로 방향 해상도
									 wr.bottom - wr.top, // 윈도우 세로 방향 해상도
									 NULL, NULL, wc.hInstance, NULL);

		if (!m_hMainWindow)
		{
			__debugbreak();
		}

		ShowWindow(m_hMainWindow, SW_SHOWDEFAULT);
		UpdateWindow(m_hMainWindow);
	}

	void BaseRenderer::initDirect3D()
	{
		HRESULT hr = S_OK;

		const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_UNKNOWN;
		// const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;
		// const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_WARP;

		UINT createDeviceFlags = 0;
		UINT createFactoryFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
		createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

		const D3D_FEATURE_LEVEL FEATURE_LEVELS[] =
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
		};
		UINT numFeatureLevels = _countof(FEATURE_LEVELS);
		IDXGIFactory5* pFactory = nullptr;
		IDXGIAdapter3* pAdapter = nullptr;

		hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&pFactory));
		BREAK_IF_FAILED(hr);

		for (UINT featureLevelIndex = 0; featureLevelIndex < numFeatureLevels; ++featureLevelIndex)
		{
			UINT adapterIndex = 0;
			while (pFactory->EnumAdapters1(adapterIndex, (IDXGIAdapter1**)&pAdapter) != DXGI_ERROR_NOT_FOUND)
			{
				pAdapter->GetDesc2(&m_AdapterDesc);
				hr = D3D11CreateDevice(pAdapter,
									   driverType,
									   nullptr,
									   0,
									   FEATURE_LEVELS, numFeatureLevels,
									   D3D11_SDK_VERSION,
									   &m_pDevice, &m_FeatureLevel, &m_pContext);
				if (SUCCEEDED(hr))
				{
					m_pDevice->QueryInterface(IID_PPV_ARGS(&m_pDevice5));
					m_pContext->QueryInterface(IID_PPV_ARGS(&m_pContext4));
					goto LB_EXIT;
				}

				RELEASE(pAdapter);
				++adapterIndex;
			}
		}
	LB_EXIT:
		BREAK_IF_FAILED(hr);

		{
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
			swapChainDesc.Width = m_ScreenWidth;
			swapChainDesc.Height = m_ScreenHeight;
			// swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
			//swapChainDesc.BufferDesc.RefreshRate.Numerator = m_uiRefreshRate;
			//swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
			swapChainDesc.BufferCount = 2;
			if (m_bUseMSAA && m_NumQualityLevels)
			{
				swapChainDesc.SampleDesc.Count = 4;
				swapChainDesc.SampleDesc.Quality = m_NumQualityLevels - 1;
			}
			else
			{
				swapChainDesc.SampleDesc.Count = 1;
				swapChainDesc.SampleDesc.Quality = 0;
			}
			swapChainDesc.Scaling = DXGI_SCALING_NONE;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			// swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

			DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
			fsSwapChainDesc.RefreshRate.Numerator = 60;
			fsSwapChainDesc.RefreshRate.Denominator = 1;
			fsSwapChainDesc.Windowed = TRUE;

			IDXGISwapChain1* pSwapChain1 = nullptr;
			hr = pFactory->CreateSwapChainForHwnd(m_pDevice5, m_hMainWindow, &swapChainDesc, &fsSwapChainDesc, nullptr, &pSwapChain1);
			BREAK_IF_FAILED(hr);

			pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain4));
			pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain));
			RELEASE(pSwapChain1);
		}

		Graphics::InitCommonStates(m_pDevice5);
		createBuffers();
		setMainViewport();

		// 공통으로 쓰이는 ConstBuffers.
		hr = Graphics::CreateConstBuffer(m_pDevice5, m_GlobalConstsCPU, &m_pGlobalConstsGPU);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pGlobalConstsGPU, "m_pGlobalConstsGPU");

		hr = Graphics::CreateConstBuffer(m_pDevice5, m_ReflectGlobalConstsCPU, &m_pReflectGlobalConstsGPU);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pReflectGlobalConstsGPU, "m_pReflectGlobalConstsGPU");

		// 그림자맵 렌더링할 때 사용할 GlobalConsts들 별도 생성.
		for (int i = 0; i < MAX_LIGHTS; ++i)
		{
			hr = Graphics::CreateConstBuffer(m_pDevice5, m_pShadowGlobalConstsCPUs[i], &m_ppShadowGlobalConstsGPUs[i]);
			BREAK_IF_FAILED(hr);
#ifdef _DEBUG
			std::string debugName("m_ppShadowGlobalConstsGPUs[");
			debugName += std::to_string(i) + "]";
			SET_DEBUG_INFO_TO_OBJECT(m_ppShadowGlobalConstsGPUs[i], debugName.c_str());
#endif
		}

		m_PostProcessor.Initialize(m_pDevice5, m_pContext4,
								   { m_pGlobalConstsGPU, m_pBackBuffer, m_pFloatBuffer, m_pResolvedBuffer, m_pPrevBuffer, m_pBackBufferRTV, m_pResolvedSRV, m_pPrevSRV, m_pDepthOnlySRV },
								   m_ScreenWidth, m_ScreenHeight, 4);

		RELEASE(pFactory);
		RELEASE(pAdapter);
	}

	void BaseRenderer::initGUI()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.DisplaySize = ImVec2((float)m_ScreenWidth, (float)m_ScreenHeight);
		ImGui::StyleColorsLight();

		// Setup Platform/Renderer backends
		if (!ImGui_ImplDX11_Init(m_pDevice5, m_pContext4))
		{
			__debugbreak();
		}

		if (!ImGui_ImplWin32_Init(m_hMainWindow))
		{
			__debugbreak();
		}

		// ImGuiViewport;
	}

	void BaseRenderer::createBuffers()
	{
		// 레스터화 -> float/depthBuffer(MSAA) -> resolved -> backBuffer
		HRESULT hr = S_OK;

		// BackBuffer는 화면으로 최종 출력. (SRV는 불필요)
		// ID3D11Texture2D* pBackBuffer = nullptr;
		hr = m_pSwapChain4->GetBuffer(0, IID_PPV_ARGS(&m_pBackBuffer));
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pBackBuffer, "m_pBackBuffer");

		hr = m_pDevice5->CreateRenderTargetView(m_pBackBuffer, nullptr, &m_pBackBufferRTV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pBackBufferRTV, "m_pBackBufferRTV");

		// FLOAT MSAA RenderTargetView/ShaderResourceView.
		hr = m_pDevice5->CheckMultisampleQualityLevels(DXGI_FORMAT_R16G16B16A16_FLOAT, 4, &m_NumQualityLevels);
		BREAK_IF_FAILED(hr);

		D3D11_TEXTURE2D_DESC desc = { 0, };
		m_pBackBuffer->GetDesc(&desc);
		// RELEASE(pBackBuffer);
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		// 이전 프레임 저장용.
		hr = m_pDevice5->CreateTexture2D(&desc, nullptr, &m_pPrevBuffer);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pPrevBuffer, "m_pPrevBuffer");

		hr = m_pDevice5->CreateRenderTargetView(m_pPrevBuffer, nullptr, &m_pPrevRTV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pPrevRTV, "m_pPrevRTV");

		hr = m_pDevice5->CreateShaderResourceView(m_pPrevBuffer, nullptr, &m_pPrevSRV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pPrevSRV, "m_pPrevSRV");

		desc.MipLevels = desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		desc.Usage = D3D11_USAGE_DEFAULT; // 스테이징 텍스춰로부터 복사 가능.
		desc.MiscFlags = 0;
		desc.CPUAccessFlags = 0;
		if (m_bUseMSAA && m_NumQualityLevels)
		{
			desc.SampleDesc.Count = 4;
			desc.SampleDesc.Quality = m_NumQualityLevels - 1;
		}
		else
		{
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
		}
		hr = m_pDevice5->CreateTexture2D(&desc, nullptr, &m_pFloatBuffer);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pFloatBuffer, "m_pFloatBuffer");

		hr = m_pDevice5->CreateRenderTargetView(m_pFloatBuffer, nullptr, &m_pFloatRTV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pFloatRTV, "m_pFloatRTV");

		// FLOAT MSAA를 Relsolve해서 저장할 SRV/RTV.
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		hr = m_pDevice5->CreateTexture2D(&desc, nullptr, &m_pResolvedBuffer);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pResolvedBuffer, "m_pResolvedBuffer");

		hr = m_pDevice5->CreateShaderResourceView(m_pResolvedBuffer, nullptr, &m_pResolvedSRV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pResolvedSRV, "m_pResolvedSRV");

		hr = m_pDevice5->CreateRenderTargetView(m_pResolvedBuffer, nullptr, &m_pResolvedRTV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pResolvedRTV, "m_pResolvedRTV");

		createDepthBuffers();
	}

	void BaseRenderer::createDepthBuffers()
	{
		HRESULT hr = S_OK;

		// DepthStencilView 만들기
		D3D11_TEXTURE2D_DESC desc = { 0, };
		desc.Width = m_ScreenWidth;
		desc.Height = m_ScreenHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		if (m_bUseMSAA && m_NumQualityLevels > 0)
		{
			desc.SampleDesc.Count = 4;
			desc.SampleDesc.Quality = m_NumQualityLevels - 1;
		}
		else
		{
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
		}
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

		ID3D11Texture2D* pDepthStencilBuffer = nullptr;
		hr = m_pDevice5->CreateTexture2D(&desc, nullptr, &pDepthStencilBuffer);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(pDepthStencilBuffer, "pDepthStencilBuffer");

		hr = m_pDevice5->CreateDepthStencilView(pDepthStencilBuffer, nullptr, &m_pDefaultDSV);
		RELEASE(pDepthStencilBuffer);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pDefaultDSV, "m_pDefaultDSV");

		// Depth 전용.
		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		hr = m_pDevice5->CreateTexture2D(&desc, nullptr, &m_pDepthOnlyBuffer);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pDepthOnlyBuffer, "m_pDepthOnlyBuffer");

		// 그림자 Buffers (Depth 전용).
		desc.Width = m_ShadowWidth;
		desc.Height = m_ShadowHeight;

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		ZeroMemory(&dsvDesc, sizeof(dsvDesc));
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		hr = m_pDevice5->CreateDepthStencilView(m_pDepthOnlyBuffer, &dsvDesc, &m_pDepthOnlyDSV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pDepthOnlyDSV, "m_pDepthOnlyDSV");

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		hr = m_pDevice5->CreateShaderResourceView(m_pDepthOnlyBuffer, &srvDesc, &m_pDepthOnlySRV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(m_pDepthOnlySRV, "m_pDepthOnlySRV");

		for (int i = 0; i < MAX_LIGHTS; ++i)
		{
			hr = m_pDevice5->CreateTexture2D(&desc, nullptr, &m_ppShadowBuffers[i]);
			BREAK_IF_FAILED(hr);

			// 그림자 DSVs.
			hr = m_pDevice5->CreateDepthStencilView(m_ppShadowBuffers[i], &dsvDesc, &m_ppShadowDSVs[i]);
			BREAK_IF_FAILED(hr);

			// 그림자 SRVs.
			hr = m_pDevice5->CreateShaderResourceView(m_ppShadowBuffers[i], &srvDesc, &m_ppShadowSRVs[i]);
			BREAK_IF_FAILED(hr);

#ifdef _DEBUG
			std::string debugStringPost(std::to_string(i) + "]");
			std::string debugStringPre("m_ppShadowBuffers[");

			debugStringPre += debugStringPost;
			SET_DEBUG_INFO_TO_OBJECT(m_ppShadowBuffers[i], debugStringPre.c_str());

			debugStringPre = "m_ppShadowDSVs[" + debugStringPost;
			SET_DEBUG_INFO_TO_OBJECT(m_ppShadowDSVs[i], debugStringPre.c_str());

			debugStringPre = "m_ppShadowSRVs[" + debugStringPost;
			SET_DEBUG_INFO_TO_OBJECT(m_ppShadowSRVs[i], debugStringPre.c_str());
#endif
		}
	}

	void BaseRenderer::setMainViewport()
	{
		// Set the viewport
		m_ScreenViewport = { 0, };
		m_ScreenViewport.TopLeftX = 0;
		m_ScreenViewport.TopLeftY = 0;
		m_ScreenViewport.Width = (float)m_ScreenWidth;
		m_ScreenViewport.Height = (float)m_ScreenHeight;
		m_ScreenViewport.MinDepth = 0.0f;
		m_ScreenViewport.MaxDepth = 1.0f;

		m_pContext4->RSSetViewports(1, &m_ScreenViewport);
	}

	void BaseRenderer::setShadowViewport()
	{
		// Set the viewport.
		D3D11_VIEWPORT shadowViewport = { 0, };
		shadowViewport.TopLeftX = 0;
		shadowViewport.TopLeftY = 0;
		shadowViewport.Width = (float)m_ShadowWidth;
		shadowViewport.Height = (float)m_ShadowHeight;
		shadowViewport.MinDepth = 0.0f;
		shadowViewport.MaxDepth = 1.0f;

		m_pContext4->RSSetViewports(1, &shadowViewport);
	}

	void BaseRenderer::setComputeShaderBarrier()
	{
		// 예제들에서 최대 사용하는 SRV, UAV 갯수가 6개.
		ID3D11ShaderResourceView* ppNullSRVs[6] = { nullptr, };
		ID3D11UnorderedAccessView* ppNullUAVs[6] = { nullptr, };
		m_pContext4->CSSetShaderResources(0, 6, ppNullSRVs);
		m_pContext4->CSSetUnorderedAccessViews(0, 6, ppNullUAVs, nullptr);
	}

	void BaseRenderer::destroyBuffersForRendering()
	{
		// swap chain에 사용될 back bufffer와 관련된 모든 버퍼를 초기화.
		RELEASE(m_pBackBuffer);
		RELEASE(m_pBackBufferRTV);
		RELEASE(m_pPrevBuffer);
		RELEASE(m_pPrevRTV);
		RELEASE(m_pPrevSRV);
		RELEASE(m_pFloatBuffer);
		RELEASE(m_pFloatRTV);
		RELEASE(m_pResolvedBuffer);
		RELEASE(m_pResolvedSRV);
		RELEASE(m_pResolvedRTV);
		RELEASE(m_pDefaultDSV);
		RELEASE(m_pDepthOnlyBuffer);
		RELEASE(m_pDepthOnlyDSV);
		RELEASE(m_pDepthOnlySRV);
		for (int i = 0; i < MAX_LIGHTS; ++i)
		{
			RELEASE(m_ppShadowBuffers[i]);
			RELEASE(m_ppShadowDSVs[i]);
			RELEASE(m_ppShadowSRVs[i]);
		}
	}
}
