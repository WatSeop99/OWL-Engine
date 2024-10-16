#include "../Common.h"
#include "../Graphics/Atmosphere/AerialLUT.h"
#include "../Graphics/Camera.h"
#include "GBuffer.h"
#include "../Geometry/GeometryGenerator.h"
#include "../Graphics/Light.h"
#include "../Geometry/Mesh.h"
#include "../Geometry/MeshInfo.h"
#include "../Geometry/Model.h"
#include "ResourceManager.h"
#include "../Graphics/Atmosphere/Sky.h"
#include "../Graphics/Atmosphere/SkyLUT.h"
#include "../Graphics/Atmosphere/Sun.h"
#include "../Graphics/Scene.h"
#include "PipelineState.h"
#include "../Renderer/PostProcessor.h"
#include "Texture.h"
#include "Timer.h"
#include "BaseRenderer.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using DirectX::BoundingSphere;
using DirectX::SimpleMath::Quaternion;
using DirectX::SimpleMath::Ray; BaseRenderer* g_pAppBase = nullptr;
using DirectX::SimpleMath::Vector3;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return g_pAppBase->MsgProc(hWnd, msg, wParam, lParam);
}

BaseRenderer::BaseRenderer()
{
	g_pAppBase = this;
}

BaseRenderer::~BaseRenderer()
{
	g_pAppBase = nullptr;
	m_pScene = nullptr;
	m_pCursorSphere = nullptr;

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	m_pContext->OMSetRenderTargets(0, nullptr, nullptr);
	m_pContext->Flush();
	
	if (m_pFloatBuffer)
	{
		delete m_pFloatBuffer;
		m_pFloatBuffer = nullptr;
	}
	if (m_pPrevBuffer)
	{
		delete m_pPrevBuffer;
		m_pPrevBuffer = nullptr;
	}
	if (m_pGBuffer)
	{
		delete m_pGBuffer;
		m_pGBuffer = nullptr;
	}
	if (m_pBackBuffer)
	{
		delete m_pBackBuffer;
		m_pBackBuffer = nullptr;
	}
	if (m_pMainCamera)
	{
		delete m_pMainCamera;
		m_pMainCamera = nullptr;
	}
	if (m_pPostProcessor)
	{
		delete m_pPostProcessor;
		m_pPostProcessor = nullptr;
	}
	if (m_pResourceManager)
	{
		delete m_pResourceManager;
		m_pResourceManager = nullptr;
	}
	if (m_pTimer)
	{
		delete m_pTimer;
		m_pTimer = nullptr;
	}

	SAFE_RELEASE(m_pSwapChain);
	SAFE_RELEASE(m_pContext);
	SAFE_RELEASE(m_pDevice);

	DestroyWindow(m_hMainWindow);
}

void BaseRenderer::Initialize(Scene* const pScene)
{
	_ASSERT(pScene);

	m_pScene = pScene;

	initMainWindow();
	initDirect3D();
	initGUI();

	m_pMainCamera = new Camera;
	m_pMainCamera->SetAspectRatio(GetAspectRatio());
	m_pMainCamera->SetFarZ(500.0f);

	m_pResourceManager = new ResourceManager;
	m_pResourceManager->Initialize(m_pDevice, m_pContext);

	// postprocessor 초기화.
	m_pPostProcessor = new PostProcessor;
	m_pPostProcessor->Initialize(this,
							   { m_pBackBuffer, m_pFloatBuffer, m_pPrevBuffer, &m_pGBuffer->DepthBuffer },
							   m_ScreenWidth, m_ScreenHeight, 4);

	// Timer setting.
	m_pTimer = new Timer;
	m_pTimer->Initialize(m_pDevice, m_pContext);

	// 콘솔창이 렌더링 창을 덮는 것을 방지.
	SetForegroundWindow(m_hMainWindow);

	m_DeltaTimeData.resize(90, 0);
	m_FrameRateData.resize(90, 0);
}

void BaseRenderer::InitScene()
{
	_ASSERT(m_pScene);

	m_pPostProcessor->SetGlobalConstants(m_pScene->GetGlobalConstantBufferPtr());

	// 커서 표시 (Main sphere와의 충돌이 감지되면 월드 공간에 작게 그려지는 구).
	{
		MeshInfo sphere;
		MakeSphere(&sphere, 0.01f, 10, 10);

		m_pCursorSphere = new Model;
		m_pCursorSphere->Initialize(this, { sphere });
		m_pCursorSphere->bIsVisible = false; // 마우스가 눌렸을 때만 보임
		m_pCursorSphere->bCastShadow = false; // 그림자 X

		MaterialConstants* pMaterialConstData = (MaterialConstants*)m_pCursorSphere->Meshes[0]->MaterialConstant.pSystemMem;
		pMaterialConstData->AlbedoFactor = Vector3(0.0f);
		pMaterialConstData->EmissionFactor = Vector3(0.0f, 1.0f, 0.0f);

		m_pScene->RenderObjects.push_back(m_pCursorSphere);
	}
}

void BaseRenderer::UpdateGUI()
{
	_ASSERT(m_DeltaTimeData.size() > 0);
	_ASSERT(m_FrameRateData.size() > 0);

	static int s_DeltaTimeIndex = 0;
	static int s_FrameIndex = 0;

	const ImGuiIO& IMGUI_IO = ImGui::GetIO();

	ImGui::Begin("Profile");

	if (s_DeltaTimeIndex == m_DeltaTimeData.size())
	{
		s_DeltaTimeIndex = 0;
	}
	m_DeltaTimeData[s_DeltaTimeIndex++] = IMGUI_IO.DeltaTime;
	ImGui::PushID("dt");
	ImGui::Text("%s\t%-3.4f %s", "dt", IMGUI_IO.DeltaTime, "ms");
	ImGui::PlotLines("##plotvar", m_DeltaTimeData.data(), (int)m_DeltaTimeData.size(), s_DeltaTimeIndex, nullptr, FLT_MAX, FLT_MAX, { 0, 50 });
	ImGui::PopID();


	if (s_FrameIndex == m_FrameRateData.size())
	{
		s_FrameIndex = 0;
	}
	m_FrameRateData[s_FrameIndex++] = IMGUI_IO.Framerate;
	ImGui::PushID("fps");
	ImGui::Text("%s\t%-3.4f", "fps", IMGUI_IO.Framerate);
	ImGui::PlotLines("##plotvar", m_FrameRateData.data(), (int)m_FrameRateData.size(), s_FrameIndex, nullptr, FLT_MAX, FLT_MAX, { 0, 50 });
	ImGui::PopID();

	ImGui::End();
}

void BaseRenderer::Update(float deltaTime)
{
	_ASSERT(m_pMainCamera);
	_ASSERT(m_pScene);

	// 카메라의 이동.
	m_pMainCamera->UpdateKeyboard(deltaTime, &m_Keyboard);

	// 마우스 처리.
	ProcessMouseControl();

	// 전체 씬 업데이트.
	m_pScene->Update(deltaTime);

	// 후처리 프로세서 업데이트.
	m_pPostProcessor->Update();
}

void BaseRenderer::RenderGUI()
{
	ImGui::Begin("Scene");
	ImVec2 wsize = ImGui::GetWindowSize();
	ImGui::Image((ImTextureID)(intptr_t)m_pPrevBuffer->pSRV, wsize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
	ImGui::End();

	setMainViewport();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void BaseRenderer::Render()
{
	m_pContext->VSSetSamplers(0, (UINT)m_pResourceManager->SamplerStates.size(), m_pResourceManager->SamplerStates.data());
	m_pContext->PSSetSamplers(0, (UINT)m_pResourceManager->SamplerStates.size(), m_pResourceManager->SamplerStates.data());

	passGBuffer();
	passShadow();

	m_pScene->GetSkyLUTPtr()->Generate();
	m_pScene->GetAerialLUTPtr()->Generate();

	passDeferredLighting();
	passSky();
	passDebug();

	m_pPostProcessor->Render();

	RenderGUI();

	m_pSwapChain->Present(1, 0);
}

void BaseRenderer::OnMouseMove(int mouseX, int mouseY)
{
	_ASSERT(m_pMainCamera);

	m_Mouse.MouseX = mouseX;
	m_Mouse.MouseY = mouseY;

	// 마우스 커서의 위치를 NDC로 변환.
	// 마우스 커서는 좌측 상단 (0, 0), 우측 하단(width-1, height-1).
	// NDC는 좌측 하단이 (-1, -1), 우측 상단(1, 1).
	m_Mouse.MouseNDCX = mouseX * 2.0f / m_ScreenWidth - 1.0f;
	m_Mouse.MouseNDCY = -mouseY * 2.0f / m_ScreenHeight + 1.0f;

	// 커서가 화면 밖으로 나갔을 경우 범위 조절.
	m_Mouse.MouseNDCX = Clamp(m_Mouse.MouseNDCX, -1.0f, 1.0f);
	m_Mouse.MouseNDCY = Clamp(m_Mouse.MouseNDCY, -1.0f, 1.0f);

	// 카메라 시점 회전.
	m_pMainCamera->UpdateMouse(m_Mouse.MouseNDCX, m_Mouse.MouseNDCY);
}

void BaseRenderer::OnMouseClick(int mouseX, int mouseY)
{
	m_Mouse.MouseX = mouseX;
	m_Mouse.MouseY = mouseY;

	m_Mouse.MouseNDCX = mouseX * 2.0f / m_ScreenWidth - 1.0f;
	m_Mouse.MouseNDCY = -mouseY * 2.0f / m_ScreenHeight + 1.0f;
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
		{
			// 화면 해상도가 바뀌면 SwapChain을 다시 생성.
			if (m_pSwapChain)
			{
				_ASSERT(m_pMainCamera);
				_ASSERT(m_pScene);

				m_ScreenWidth = (int)LOWORD(lParam);
				m_ScreenHeight = (int)HIWORD(lParam);

				// 윈도우가 Minimize 모드에서는 screenWidth/Height가 0.
				if (m_ScreenWidth && m_ScreenHeight)
				{
#ifdef _DEBUG
					char debugString[256];
					sprintf(debugString, "Resize SwapChain to %d %d\n", m_ScreenWidth, m_ScreenHeight);
					OutputDebugStringA(debugString);
#endif 

					// 기존 버퍼 초기화.
					destroyBuffersForRendering();
					m_pSwapChain->ResizeBuffers(0, m_ScreenWidth, m_ScreenHeight, DXGI_FORMAT_UNKNOWN, 0);

					createBuffers();
					m_pMainCamera->SetAspectRatio(GetAspectRatio());
					m_pPostProcessor->Initialize(this,
											   { m_pBackBuffer, m_pFloatBuffer, m_pPrevBuffer, &m_pGBuffer->DepthBuffer },
											   m_ScreenWidth, m_ScreenHeight, 4);
					m_pPostProcessor->SetGlobalConstants(m_pScene->GetGlobalConstantBufferPtr());
				}
			}

			break;
		}

		case WM_SYSCOMMAND:
		{
			if ((wParam & 0xFFF0) == SC_KEYMENU) // ALT키 비활성화.
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
			if (!m_Mouse.bMouseLeftButton)
			{
				m_Mouse.bMouseDragStartFlag = true; // 드래그를 새로 시작하는지 확인.
			}
			m_Mouse.bMouseLeftButton = true;
			OnMouseClick(LOWORD(lParam), HIWORD(lParam));

			break;
		}

		case WM_LBUTTONUP:
			m_Mouse.bMouseLeftButton = false;
			break;

		case WM_RBUTTONDOWN:
		{
			if (!m_Mouse.bMouseRightButton)
			{
				m_Mouse.bMouseDragStartFlag = true; // 드래그를 새로 시작하는지 확인.
			}
			m_Mouse.bMouseRightButton = true;

			break;
		}

		case WM_RBUTTONUP:
			m_Mouse.bMouseRightButton = false;
			break;

		case WM_KEYDOWN:
		{
			m_Keyboard.bPressed[wParam] = true;
			if (wParam == VK_ESCAPE) // ESC키 종료.
			{
				DestroyWindow(hwnd);
			}

			break;
		}

		case WM_KEYUP:
		{
			if (wParam == 'F')  // f키 일인칭 시점.
			{
				_ASSERT(m_pMainCamera);
				m_pMainCamera->bUseFirstPersonView = !m_pMainCamera->bUseFirstPersonView;
			}
			if (wParam == 'P') // 애니메이션 일시중지할 때 사용.
			{
				m_bPauseAnimation = !m_bPauseAnimation;
			}
			if (wParam == 'Z') // 카메라 설정 화면에 출력.
			{
				_ASSERT(m_pMainCamera);
				m_pMainCamera->PrintView();
			}

			m_Keyboard.bPressed[wParam] = false;
			break;
		}

		case WM_MOUSEWHEEL:
			m_Mouse.WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		default:
			break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void BaseRenderer::SetGlobalConsts(ID3D11Buffer** ppGlobalConstsGPU, UINT slot)
{
	// 쉐이더와 일관성 유지 cbuffer GlobalConstants : register(b0).
	m_pContext->VSSetConstantBuffers(slot, 1, ppGlobalConstsGPU);
	m_pContext->PSSetConstantBuffers(slot, 1, ppGlobalConstsGPU);
	m_pContext->GSSetConstantBuffers(slot, 1, ppGlobalConstsGPU);
}

void BaseRenderer::SetViewport(const D3D11_VIEWPORT* pViewports, const UINT NUM_VIEWPORT)
{
	_ASSERT(pViewports);
	_ASSERT(NUM_VIEWPORT > 0);

	m_pContext->RSSetViewports(NUM_VIEWPORT, pViewports);
}

void BaseRenderer::SetPipelineState(const GraphicsPSO* pPSO)
{
	m_pContext->VSSetShader(pPSO->pVertexShader, nullptr, 0);
	m_pContext->PSSetShader(pPSO->pPixelShader, nullptr, 0);
	m_pContext->HSSetShader(pPSO->pHullShader, nullptr, 0);
	m_pContext->DSSetShader(pPSO->pDomainShader, nullptr, 0);
	m_pContext->GSSetShader(pPSO->pGeometryShader, nullptr, 0);
	m_pContext->CSSetShader(nullptr, nullptr, 0);
	m_pContext->IASetInputLayout(pPSO->pInputLayout);
	m_pContext->RSSetState(pPSO->pRasterizerState);
	m_pContext->OMSetBlendState(pPSO->pBlendState, pPSO->BlendFactor, 0xffffffff);
	m_pContext->OMSetDepthStencilState(pPSO->pDepthStencilState, pPSO->StencilRef);
	m_pContext->IASetPrimitiveTopology(pPSO->PrimitiveTopology);
}

void BaseRenderer::SetPipelineState(const ComputePSO* pPSO)
{
	m_pContext->VSSetShader(nullptr, nullptr, 0);
	m_pContext->PSSetShader(nullptr, nullptr, 0);
	m_pContext->HSSetShader(nullptr, nullptr, 0);
	m_pContext->DSSetShader(nullptr, nullptr, 0);
	m_pContext->GSSetShader(nullptr, nullptr, 0);
	m_pContext->CSSetShader(pPSO->pComputeShader, nullptr, 0);
}

Model* BaseRenderer::PickClosest(const DirectX::SimpleMath::Ray* pPickingRay, float* pMinDist)
{
	_ASSERT(m_pScene);

	*pMinDist = 1e5f;
	Model* pMinModel = nullptr;

	for (UINT64 i = 0, size = m_pScene->RenderObjects.size(); i < size; ++i)
	{
		Model* pCurModel = m_pScene->RenderObjects[i];
		float dist = 0.0f;
		if (pCurModel->bIsPickable &&
			pPickingRay->Intersects(pCurModel->BoundingSphere, dist) &&
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
	_ASSERT(m_pMainCamera);

	static Model* s_pActiveModel = nullptr;
	static float s_PrevRatio = 0.0f;
	static Vector3 s_PrevPos(0.0f);
	static Vector3 s_PrevVector(0.0f);

	// 적용할 회전과 이동 초기화.
	Quaternion dragRotation = Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), 0.0f);
	Vector3 dragTranslation(0.0f);
	Vector3 pickPoint(0.0f);
	float dist = 0.0f;

	// 사용자가 두 버튼 중 하나만 누른다고 가정.
	if (m_Mouse.bMouseLeftButton || m_Mouse.bMouseRightButton)
	{
		const Matrix VIEW = m_pMainCamera->GetView();
		const Matrix PROJECTION = m_pMainCamera->GetProjection();
		const Vector3 NDC_NEAR = Vector3(m_Mouse.MouseNDCX, m_Mouse.MouseNDCY, 0.0f);
		const Vector3 NDC_FAR = Vector3(m_Mouse.MouseNDCX, m_Mouse.MouseNDCY, 1.0f);
		const Matrix INV_PROJECTION_VIEW = (VIEW * PROJECTION).Invert();
		const Vector3 WORLD_NEAR = Vector3::Transform(NDC_NEAR, INV_PROJECTION_VIEW);
		const Vector3 WORLD_FAR = Vector3::Transform(NDC_FAR, INV_PROJECTION_VIEW);
		Vector3 dir = WORLD_FAR - WORLD_NEAR;
		dir.Normalize();
		const Ray CUR_RAY = DirectX::SimpleMath::Ray(WORLD_NEAR, dir);

		if (s_pActiveModel == nullptr) // 이전 프레임에서 아무 물체도 선택되지 않았을 경우에는 새로 선택.
		{
			Model* pSelectedModel = PickClosest(&CUR_RAY, &dist);
			if (pSelectedModel)
			{
#ifdef _DEBUG
				char szDebugString[256];
				sprintf_s(szDebugString, 256, "newly selected model: %s\n", pSelectedModel->Name.c_str());
				OutputDebugStringA(szDebugString);
#endif

				s_pActiveModel = pSelectedModel;
				m_pPickedModel = pSelectedModel; // GUI 조작용 포인터.
				pickPoint = CUR_RAY.position + dist * CUR_RAY.direction;
				if (m_Mouse.bMouseLeftButton) // 왼쪽 버튼 회전 준비.
				{
					s_PrevVector = pickPoint - s_pActiveModel->BoundingSphere.Center;
					s_PrevVector.Normalize();
				}
				else
				{ 
					// 오른쪽 버튼 이동 준비
					m_Mouse.bMouseDragStartFlag = false;
					s_PrevRatio = dist / (WORLD_FAR - WORLD_NEAR).Length();
					s_PrevPos = pickPoint;
				}
			}
		}
		else // 이미 선택된 물체가 있었던 경우.
		{
			if (m_Mouse.bMouseLeftButton) // 왼쪽 버튼으로 계속 회전.
			{
				if (CUR_RAY.Intersects(s_pActiveModel->BoundingSphere, dist))
				{
					pickPoint = CUR_RAY.position + dist * CUR_RAY.direction;
				}
				else // 바운딩 스피어에 가장 가까운 점을 찾기.
				{
					Vector3 c = s_pActiveModel->BoundingSphere.Center - WORLD_NEAR;
					Vector3 centerToRay = dir.Dot(c) * dir - c;
					pickPoint = c + centerToRay * Clamp(s_pActiveModel->BoundingSphere.Radius / centerToRay.Length(), 0.0f, 1.0f);
					pickPoint += WORLD_NEAR;
				}

				Vector3 currentVector = pickPoint - s_pActiveModel->BoundingSphere.Center;
				currentVector.Normalize();
				float theta = acos(s_PrevVector.Dot(currentVector));
				if (theta > DirectX::XM_PI / 180.0f * 3.0f)
				{
					Vector3 axis = s_PrevVector.Cross(currentVector);
					axis.Normalize();
					dragRotation = Quaternion::CreateFromAxisAngle(axis, theta);
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

	if (s_pActiveModel)
	{
		Vector3 translation = s_pActiveModel->World.Translation();
		s_pActiveModel->World.Translation(Vector3(0.0f));
		s_pActiveModel->UpdateWorld(s_pActiveModel->World * Matrix::CreateFromQuaternion(dragRotation) * Matrix::CreateTranslation(dragTranslation + translation));
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

	RECT wr = { 0, 0, (long)m_ScreenWidth, (long)m_ScreenHeight };
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

	const D3D_DRIVER_TYPE DRIVER_TYPE = D3D_DRIVER_TYPE_UNKNOWN;
	// const D3D_DRIVER_TYPE DRIVER_TYPE = D3D_DRIVER_TYPE_HARDWARE;
	// const D3D_DRIVER_TYPE DRIVER_TYPE = D3D_DRIVER_TYPE_WARP;

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
			ID3D11Device* pDevice = nullptr;
			ID3D11DeviceContext* pContext = nullptr;

			pAdapter->GetDesc2(&m_AdapterDesc);
			hr = D3D11CreateDevice(pAdapter,
								   DRIVER_TYPE,
								   nullptr,
								   0,
								   FEATURE_LEVELS, numFeatureLevels,
								   D3D11_SDK_VERSION,
								   &pDevice, &m_FeatureLevel, &pContext);
			if (SUCCEEDED(hr))
			{
				pDevice->QueryInterface(IID_PPV_ARGS(&m_pDevice));
				pContext->QueryInterface(IID_PPV_ARGS(&m_pContext));
				pDevice->Release();
				pContext->Release();

				goto LB_EXIT;
			}

			SAFE_RELEASE(pDevice);
			SAFE_RELEASE(pContext);
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
		//swapChainDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//swapChainDesc.BufferDesc.RefreshRate.Numerator = m_uiRefreshRate;
		//swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_UNORDERED_ACCESS;
		swapChainDesc.BufferCount = 2;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
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
		hr = pFactory->CreateSwapChainForHwnd(m_pDevice, m_hMainWindow, &swapChainDesc, &fsSwapChainDesc, nullptr, &pSwapChain1);
		BREAK_IF_FAILED(hr);

		pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain));
		pSwapChain1->Release();

		m_BackBufferFormat = swapChainDesc.Format;
	}

	createBuffers();
	setMainViewport();

	pFactory->Release();
	pAdapter->Release();
}

void BaseRenderer::initGUI()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)m_ScreenWidth, (float)m_ScreenHeight);
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// Setup Platform/Renderer backends
	if (!ImGui_ImplDX11_Init(m_pDevice, m_pContext))
	{
		__debugbreak();
	}
	if (!ImGui_ImplWin32_Init(m_hMainWindow))
	{
		__debugbreak();
	}
}

void BaseRenderer::createBuffers()
{
	HRESULT hr = S_OK;

	ID3D11Texture2D* pTexture = nullptr;
	D3D11_TEXTURE2D_DESC desc = {};
	hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pTexture));
	BREAK_IF_FAILED(hr);

	if (!m_pBackBuffer)
	{
		m_pBackBuffer = new Texture;
	}
	pTexture->GetDesc(&desc);
	m_pBackBuffer->Initialize(m_pDevice, m_pContext, pTexture, true);
	RELEASE(pTexture);

	// 이전 프레임 저장용.
	(*m_pBackBuffer->GetTexture2DPPtr())->GetDesc(&desc);
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	if (!m_pPrevBuffer)
	{
		m_pPrevBuffer = new Texture;
	}
	m_pPrevBuffer->Initialize(m_pDevice, m_pContext, desc, nullptr, true);

	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT; // 스테이징 텍스춰로부터 복사 가능.
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.MiscFlags = 0;
	desc.CPUAccessFlags = 0;
	if (!m_pFloatBuffer)
	{
		m_pFloatBuffer = new Texture;
	}
	m_pFloatBuffer->Initialize(m_pDevice, m_pContext, desc, nullptr, true);

	if (!m_pGBuffer)
	{
		m_pGBuffer = new GBuffer;
	}
	m_pGBuffer->Initialize(m_pDevice, m_pContext, m_ScreenWidth, m_ScreenHeight);
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

	m_pContext->RSSetViewports(1, &m_ScreenViewport);
}

void BaseRenderer::setComputeShaderBarrier()
{
	// 예제들에서 최대 사용하는 SRV, UAV 갯수가 6개.
	ID3D11ShaderResourceView* ppNullSRVs[6] = { nullptr, };
	ID3D11UnorderedAccessView* ppNullUAVs[6] = { nullptr, };
	m_pContext->CSSetShaderResources(0, 6, ppNullSRVs);
	m_pContext->CSSetUnorderedAccessViews(0, 6, ppNullUAVs, nullptr);
}

void BaseRenderer::destroyBuffersForRendering()
{
	// swap chain에 사용될 back bufffer와 관련된 모든 버퍼를 초기화.
	m_pBackBuffer->Cleanup();
	m_pGBuffer->Cleanup();
	m_pPrevBuffer->Cleanup();
	m_pFloatBuffer->Cleanup();

	m_pPostProcessor->Cleanup();
}

void BaseRenderer::passGBuffer()
{
	_ASSERT(m_pGBuffer);
	_ASSERT(m_pScene);

	setMainViewport();
	SetGlobalConsts(&m_pScene->GetGlobalConstantBufferPtr()->pBuffer, 0);
	m_pGBuffer->PrepareRender();

	for (UINT64 i = 0, size = m_pScene->RenderObjects.size(); i < size; ++i)
	{
		Model* const pModel = m_pScene->RenderObjects[i];
		m_pResourceManager->SetPipelineState(pModel->GetGBufferPSO(false));
		pModel->Render();
	}

	m_pGBuffer->AfterRender();
}

void BaseRenderer::passShadow()
{
	_ASSERT(m_pScene);

	m_pScene->GetSunPtr()->RenderShadowMap(m_pScene->RenderObjects, nullptr);
	for (UINT64 i = 0, size = m_pScene->Lights.size(); i < size; ++i)
	{
		m_pScene->Lights[i].RenderShadowMap(m_pScene->RenderObjects, nullptr);
	}
}

void BaseRenderer::passDeferredLighting()
{
	_ASSERT(m_pScene);

	setMainViewport();
	m_pResourceManager->SetPipelineState(GraphicsPSOType_DeferredRendering);
	SetGlobalConsts(&m_pScene->GetGlobalConstantBufferPtr()->pBuffer, 0);

	const float CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_pContext->ClearRenderTargetView(m_pFloatBuffer->pRTV, CLEAR_COLOR);
	m_pContext->OMSetRenderTargets(1, &m_pFloatBuffer->pRTV, nullptr);

	ID3D11ShaderResourceView* ppSRVs[5] = { m_pGBuffer->AlbedoBuffer.pSRV, m_pGBuffer->NormalBuffer.pSRV, m_pGBuffer->PositionBuffer.pSRV, m_pGBuffer->EmissionBuffer.pSRV, m_pGBuffer->ExtraBuffer.pSRV };
	m_pContext->PSSetShaderResources(0, 5, ppSRVs);

	ConstantBuffer* pLightConstantBuffer = m_pScene->GetLightConstantBufferPtr();
	if (!pLightConstantBuffer)
	{
		__debugbreak();
	}

	LightConstants* pLightConstsData = (LightConstants*)pLightConstantBuffer->pSystemMem;
	if (!pLightConstsData)
	{
		__debugbreak();
	}

	// Draw obejct for each light.

	Sun* pSun = m_pScene->GetSunPtr();
	memcpy(&pLightConstsData->Lights, &pSun->SunProperty, sizeof(LightProperty));
	pLightConstantBuffer->Upload();
	SetGlobalConsts(&pLightConstantBuffer->pBuffer, 1);
	m_pContext->PSSetShaderResources(7, 1, &pSun->GetShadowMapPtr()->GetCascadeShadowBufferPtr()->pSRV);
	m_pContext->Draw(6, 0);

	for (UINT64 i = 0, size = m_pScene->Lights.size(); i < size; ++i)
	{
		Light& curLight = m_pScene->Lights[i];

		memcpy(&pLightConstsData->Lights, &m_pScene->Lights[i].Property, sizeof(LightProperty));
		pLightConstantBuffer->Upload();
		SetGlobalConsts(&pLightConstantBuffer->pBuffer, 1);

		switch (curLight.Property.LightType & (LIGHT_DIRECTIONAL | LIGHT_POINT | LIGHT_SPOT))
		{
			case LIGHT_DIRECTIONAL:
			case LIGHT_SPOT:
				m_pContext->PSSetShaderResources(5, 1, &curLight.GetShadowMapPtr()->GetShadow2DBufferPtr()->pSRV);
				break;

			case LIGHT_POINT:
				m_pContext->PSSetShaderResources(6, 1, &curLight.GetShadowMapPtr()->GetShadowCubeBufferPtr()->pSRV);
				break;

			default:
				break;
		}

		m_pContext->Draw(6, 0);
	}
}

void BaseRenderer::passSky()
{
	_ASSERT(m_pScene);

	setMainViewport();
	m_pContext->OMSetRenderTargets(1, &m_pFloatBuffer->pRTV, m_pGBuffer->DepthBuffer.pDSV);

	m_pScene->GetSkyPtr()->Render(m_pScene->GetSkyLUTPtr()->GetSkyLUT());
	m_pScene->GetSunPtr()->Render();

	ID3D11RenderTargetView* pNullRTV = nullptr;
	ID3D11DepthStencilView* pNullDSV = nullptr;
	m_pContext->OMSetRenderTargets(1, &pNullRTV, pNullDSV);
}

void BaseRenderer::passDebug()
{
	_ASSERT(m_pScene);

	setMainViewport();
	SetGlobalConsts(&m_pScene->GetGlobalConstantBufferPtr()->pBuffer, 0);
	m_pContext->OMSetRenderTargets(1, &m_pFloatBuffer->pRTV, m_pGBuffer->DepthBuffer.pDSV);

	for (UINT64 i = 0, size = m_pScene->RenderObjects.size(); i < size; ++i)
	{
		Model* const pModel = m_pScene->RenderObjects[i];
		if (pModel->bDrawNormals)
		{
			m_pResourceManager->SetPipelineState(GraphicsPSOType_Normal);
			pModel->RenderNormals();
		}
		if (m_pScene->bDrawOBB)
		{
			m_pResourceManager->SetPipelineState(GraphicsPSOType_BoundingBox);
			pModel->RenderWireBoundingBox();
		}
		if (m_pScene->bDrawBS)
		{
			m_pResourceManager->SetPipelineState(GraphicsPSOType_BoundingBox);
			pModel->RenderWireBoundingSphere();
		}
	}

	ID3D11RenderTargetView* pNullRTV = nullptr;
	ID3D11DepthStencilView* pNullDSV = nullptr;
	m_pContext->OMSetRenderTargets(1, &pNullRTV, pNullDSV);
}
