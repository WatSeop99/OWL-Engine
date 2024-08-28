#include "../Common.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "../Geometry/GeometryGenerator.h"
#include "BaseRenderer.h"

// imgui_impl_win32.cpp에 정의된 메시지 처리 함수에 대한 전방 선언
// Vcpkg를 통해 IMGUI를 사용할 경우 빨간줄로 경고가 뜰 수 있음
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


using namespace DirectX;
using namespace DirectX::SimpleMath;
using DirectX::BoundingSphere;
using DirectX::SimpleMath::Vector3;

BaseRenderer* g_pAppBase = nullptr;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return g_pAppBase->MsgProc(hWnd, msg, wParam, lParam);
}

BaseRenderer::BaseRenderer() : m_Scene(m_Camera, m_FloatBuffer, m_ResolvedBuffer)
{
	g_pAppBase = this;
	m_Camera.SetAspectRatio(GetAspectRatio());
	m_Camera.SetFarZ(500.0f);

	m_Scene.SetScreenWidth(m_ScreenWidth);
	m_Scene.SetScreenHeight(m_ScreenHeight);
}

BaseRenderer::~BaseRenderer()
{
	g_pAppBase = nullptr;

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	m_pContext4->OMSetRenderTargets(0, nullptr, nullptr);
	m_pContext4->Flush();

	DestroyCommonStates();

	if (m_pTimer)
	{
		delete m_pTimer;
		m_pTimer = nullptr;
	}

	m_pCursorSphere = nullptr;

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
			Render();

			// GUI 렌더링 후에 Present() 호출.
			m_pSwapChain->Present(1, 0);
		}
	}

	return (int)(msg.wParam);
}

void BaseRenderer::Initialize()
{
	initMainWindow();
	initDirect3D();
	initGUI();

	// Timer setting.
	m_pTimer = New Timer(m_pDevice5);

	_ASSERT(m_pTimer != nullptr);
	m_pTimer->Start(m_pContext4, false);

	InitScene();

	// postprocessor 초기화.
	m_PostProcessor.Initialize(m_pDevice5, m_pContext4,
							   { m_Scene.GetGlobalConstantsGPU(), m_pBackBuffer, m_FloatBuffer.pTexture, m_ResolvedBuffer.pTexture, m_PrevBuffer.pTexture, m_pBackBufferRTV, m_ResolvedBuffer.pSRV, m_PrevBuffer.pSRV, m_Scene.GetDepthOnlyBufferSRV() },
							   m_ScreenWidth, m_ScreenHeight, 4);

	// 콘솔창이 렌더링 창을 덮는 것을 방지.
	SetForegroundWindow(m_hMainWindow);

	OutputDebugStringA("Renderer intialize time ==> ");
	m_pTimer->End(m_pContext4);
}

// 여러 예제들이 공통적으로 사용하기 좋은 장면 설정
void BaseRenderer::InitScene()
{
	m_Scene.Initialize(m_pDevice5, m_pContext4);
	m_Scene.ResetBuffers(m_pDevice5, m_bUseMSAA, m_NumQualityLevels);

	// 커서 표시 (Main sphere와의 충돌이 감지되면 월드 공간에 작게 그려지는 구).
	{
		MeshInfo sphere;
		MakeSphere(&sphere, 0.01f, 10, 10);

		m_pCursorSphere = New Model;
		m_pCursorSphere->Initialize(m_pDevice5, m_pContext4, { sphere });
		m_pCursorSphere->bIsVisible = false; // 마우스가 눌렸을 때만 보임
		m_pCursorSphere->bCastShadow = false; // 그림자 X
		m_pCursorSphere->Meshes[0]->MaterialConstant.CPU.AlbedoFactor = Vector3(0.0f);
		m_pCursorSphere->Meshes[0]->MaterialConstant.CPU.EmissionFactor = Vector3(0.0f, 1.0f, 0.0f);

		m_Scene.pRenderObjects.push_back(m_pCursorSphere); // 리스트에 등록.
	}
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

void BaseRenderer::Update(float deltaTime)
{
	UpdateGUI();

	// 카메라의 이동.
	m_Camera.UpdateKeyboard(deltaTime, m_pbKeyPressed);

	// 마우스 처리.
	ProcessMouseControl();

	// 전체 씬 업데이트.
	m_Scene.Update(m_pContext4, deltaTime);

	// 후처리 프로세서 업데이트.
	m_PostProcessor.Update(m_pContext4);
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
	m_Scene.Render(m_pContext4);
	m_PostProcessor.Render(m_pContext4);
	RenderGUI(); // 추후 editor/game 모드를 설정하여 따로 렌더링하도록 구상.
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
		{
			// 화면 해상도가 바뀌면 SwapChain을 다시 생성.
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
												 m_ScreenWidth,		 // 해상도 변경.
												 m_ScreenHeight,
												 DXGI_FORMAT_UNKNOWN, // 현재 포맷 유지.
												 0);


					createBuffers();
					m_Scene.SetScreenWidth(m_ScreenWidth);
					m_Scene.SetScreenHeight(m_ScreenHeight);
					m_Scene.ResetBuffers(m_pDevice5, m_bUseMSAA, m_NumQualityLevels);
					setMainViewport();
					m_Camera.SetAspectRatio(GetAspectRatio());
					m_PostProcessor.Initialize(m_pDevice5, m_pContext4,
											   { m_Scene.GetGlobalConstantsGPU(), m_pBackBuffer, m_FloatBuffer.pTexture, m_ResolvedBuffer.pTexture, m_PrevBuffer.pTexture, m_pBackBufferRTV, m_ResolvedBuffer.pSRV, m_PrevBuffer.pSRV, m_Scene.GetDepthOnlyBufferSRV() },
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
				// m_pLights[1].bRotated = !m_pLights[1].bRotated;
				m_Scene.pLights[1].bRotated = !(m_Scene.pLights[1].bRotated);
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
				WriteToPngFile(m_pDevice5, m_pContext4, pBackBuffer, L"captured.png");
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

void BaseRenderer::SetGlobalConsts(ID3D11Buffer** ppGlobalConstsGPU, UINT slot)
{
	// 쉐이더와 일관성 유지 cbuffer GlobalConstants : register(b0).
	m_pContext4->VSSetConstantBuffers(slot, 1, ppGlobalConstsGPU);
	m_pContext4->PSSetConstantBuffers(slot, 1, ppGlobalConstsGPU);
	m_pContext4->GSSetConstantBuffers(slot, 1, ppGlobalConstsGPU);
}

void BaseRenderer::SetPipelineState(const GraphicsPSO& PSO)
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

void BaseRenderer::SetPipelineState(const ComputePSO& PSO)
{
	m_pContext4->VSSetShader(nullptr, nullptr, 0);
	m_pContext4->PSSetShader(nullptr, nullptr, 0);
	m_pContext4->HSSetShader(nullptr, nullptr, 0);
	m_pContext4->DSSetShader(nullptr, nullptr, 0);
	m_pContext4->GSSetShader(nullptr, nullptr, 0);
	m_pContext4->CSSetShader(PSO.pComputeShader, nullptr, 0);
}

Model* BaseRenderer::PickClosest(const Ray& PICKNG_RAY, float* pMinDist)
{
	*pMinDist = 1e5f;
	Model* pMinModel = nullptr;

	for (size_t i = 0, size = m_Scene.pRenderObjects.size(); i < size; ++i)
	{
		Model* pCurModel = m_Scene.pRenderObjects[i];
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

		if (s_pActiveModel == nullptr) // 이전 프레임에서 아무 물체도 선택되지 않았을 경우에는 새로 선택.
		{
			Model* pSelectedModel = PickClosest(CUR_RAY, &dist);
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
					dragRotation = SimpleMath::Quaternion::CreateFromAxisAngle(axis, theta);
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

	if (s_pActiveModel != nullptr)
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

	if (m_hMainWindow == nullptr)
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
			pAdapter->GetDesc2(&m_AdapterDesc);
			hr = D3D11CreateDevice(pAdapter,
								   DRIVER_TYPE,
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
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_UNORDERED_ACCESS;
		swapChainDesc.BufferCount = 2;
		/*if (m_bUseMSAA && m_NumQualityLevels > 0)
		{
			swapChainDesc.SampleDesc.Count = 4;
			swapChainDesc.SampleDesc.Quality = m_NumQualityLevels - 1;
		}
		else
		{
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = 0;
		}*/
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
		hr = pFactory->CreateSwapChainForHwnd(m_pDevice5, m_hMainWindow, &swapChainDesc, &fsSwapChainDesc, nullptr, &pSwapChain1);
		BREAK_IF_FAILED(hr);

		pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain4));
		pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain));
		RELEASE(pSwapChain1);

		m_BackBufferFormat = swapChainDesc.Format;
	}

	InitCommonStates(m_pDevice5);
	createBuffers();
	setMainViewport();

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
}

void BaseRenderer::createBuffers()
{
	// 레스터화 -> float/depthBuffer(MSAA) -> resolved -> backBuffer
	HRESULT hr = S_OK;

	// BackBuffer는 화면으로 최종 출력. (SRV는 불필요)
	hr = m_pSwapChain4->GetBuffer(0, IID_PPV_ARGS(&m_pBackBuffer));
	BREAK_IF_FAILED(hr);

	hr = m_pDevice5->CreateRenderTargetView(m_pBackBuffer, nullptr, &m_pBackBufferRTV);
	BREAK_IF_FAILED(hr);

	// 이전 프레임 저장용.
	D3D11_TEXTURE2D_DESC desc = {};
	m_pBackBuffer->GetDesc(&desc);
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	m_PrevBuffer.Initialize(m_pDevice5, desc);

	// FLOAT MSAA RenderTargetView/ShaderResourceView.
	hr = m_pDevice5->CheckMultisampleQualityLevels(DXGI_FORMAT_R16G16B16A16_FLOAT, 4, &m_NumQualityLevels);
	BREAK_IF_FAILED(hr);

	desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
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
	desc.Usage = D3D11_USAGE_DEFAULT; // 스테이징 텍스춰로부터 복사 가능.
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.MiscFlags = 0;
	desc.CPUAccessFlags = 0;
	m_FloatBuffer.Initialize(m_pDevice5, desc);

	// FLOAT MSAA를 Relsolve해서 저장할 SRV/RTV.
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS;
	m_ResolvedBuffer.Initialize(m_pDevice5, desc);

	m_Scene.ResetBuffers(m_pDevice5, m_bUseMSAA, m_NumQualityLevels);
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
	m_PrevBuffer.Destroy();
	m_FloatBuffer.Destroy();
	m_ResolvedBuffer.Destroy();
	m_PostProcessor.Cleanup();
}
