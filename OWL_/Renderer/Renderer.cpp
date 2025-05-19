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
#include "../Geometry/SkinnedMeshModel.h"
#include "PipelineState.h"
#include "../Renderer/PostProcessor.h"
#include "Texture.h"
#include "Timer.h"
#include "Renderer.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using DirectX::BoundingSphere;
using DirectX::SimpleMath::Quaternion;
using DirectX::SimpleMath::Ray;
using DirectX::SimpleMath::Vector3;

LRESULT Renderer::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
	{
		//return true;
		//return 0;
		return TRUE;
	}

	switch (msg)
	{
	case WM_CREATE:
	{
		CREATESTRUCT* pStruct = (CREATESTRUCT*)lParam;
		Renderer* pRenderer = (Renderer*)pStruct->lpCreateParams;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pRenderer);

		break;
	}

	case WM_SIZE:
	{
		Renderer* pRenderer = (Renderer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (!pRenderer)
		{
			__debugbreak();
		}
		pRenderer->OnResize((int)LOWORD(lParam), (int)HIWORD(lParam));

		break;
	}

	// WM_SYSCOMMAND => 이거 처리하면서 창 컨트롤이 아예 안먹힘.

	case WM_MOUSEMOVE:
	{
		Renderer* pRenderer = (Renderer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (!pRenderer)
		{
			__debugbreak();
		}
		pRenderer->OnMouseMove((int)LOWORD(lParam), (int)HIWORD(lParam));

		break;
	}

	case WM_LBUTTONDOWN:
	{
		Renderer* pRenderer = (Renderer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (!pRenderer)
		{
			__debugbreak();
		}
		pRenderer->OnMouseClick(true, true, (int)LOWORD(lParam), (int)HIWORD(lParam));

		break;
	}

	case WM_LBUTTONUP:
	{
		Renderer* pRenderer = (Renderer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (!pRenderer)
		{
			__debugbreak();
		}
		pRenderer->OnMouseClick(true, false, (int)LOWORD(lParam), (int)HIWORD(lParam));
		
		break;
	}

	case WM_RBUTTONDOWN:
	{
		Renderer* pRenderer = (Renderer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (!pRenderer)
		{
			__debugbreak();
		}
		pRenderer->OnMouseClick(false, true, (int)LOWORD(lParam), (int)HIWORD(lParam));

		break;
	}

	case WM_RBUTTONUP:
	{
		Renderer* pRenderer = (Renderer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (!pRenderer)
		{
			__debugbreak();
		}
		pRenderer->OnMouseClick(false, false, (int)LOWORD(lParam), (int)HIWORD(lParam));
		break;
	}

	case WM_KEYDOWN:
	{
		if (wParam == VK_ESCAPE) // ESC키 종료.
		{
			PostQuitMessage(0);
			break;
		}

		Renderer* pRenderer = (Renderer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (!pRenderer)
		{
			__debugbreak();
		}
		pRenderer->OnKeyboardClick(true, wParam);

		break;
	}

	case WM_KEYUP:
	{
		Renderer* pRenderer = (Renderer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (!pRenderer)
		{
			__debugbreak();
		}
		pRenderer->OnKeyboardClick(false, wParam);

		break;
	}

	case WM_MOUSEWHEEL:
	{
		Renderer* pRenderer = (Renderer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (!pRenderer)
		{
			__debugbreak();
		}
		pRenderer->OnMouseWheel(wParam);

		break;
	}

	case WM_CLOSE:
	case WM_DESTROY:
	case WM_QUIT:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	return 0;
}

bool Renderer::Initialize(HINSTANCE hInstance, Scene* const pScene)
{
	_ASSERT(pScene);

	m_hInstance = hInstance;
	m_pScene = pScene;

	InitMainWindow();
	InitD3D();
	InitGUI();

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

	m_DeltaTimeData.resize(90, 0);
	m_FrameRateData.resize(90, 0);

	return true;
}

bool Renderer::InitScene()
{
	_ASSERT(m_pScene);

	m_pPostProcessor->SetGlobalConstants(m_pScene->GetGlobalConstantBuffer());

	// 커서 표시 (Main sphere와의 충돌이 감지되면 월드 공간에 작게 그려지는 구).
	{
		MeshInfo sphere;
		MakeSphere(&sphere, 0.01f, 10, 10);

		m_pCursorSphere = new Model;
		m_pCursorSphere->Initialize(this, { sphere });
		m_pCursorSphere->bIsVisible = false; // 마우스가 눌렸을 때만 보임
		m_pCursorSphere->bCastShadow = false; // 그림자 X

		MaterialConstants* pMaterialConstData = (MaterialConstants*)m_pCursorSphere->Meshes[0]->MaterialConstant.pSystemMem;
		pMaterialConstData->AlbedoFactor = Vector3::Zero;
		pMaterialConstData->EmissionFactor = Vector3::UnitY;

		m_pScene->RenderObjects.push_back(m_pCursorSphere);
	}

	{
		/*D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = 256;
		desc.Height = 256;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R32_FLOAT;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		m_pRandomNoise = new Texture;
		m_pRandomNoise->Initialize(m_pDevice, m_pContext, desc, nullptr, true);

		m_pResourceManager->SetPipelineState(ComputePSOType_NoiseGenerate);
		m_pContext->CSSetUnorderedAccessViews(0, 1, &m_pRandomNoise->pUAV, nullptr);
		m_pContext->Dispatch(256 / 16, 256 / 16, 1);

		ID3D11UnorderedAccessView* pNullUAV = nullptr;
		m_pContext->CSSetUnorderedAccessViews(0, 1, &pNullUAV, nullptr);*/
	}

	return true;
}

void Renderer::Cleanup()
{
	m_pScene = nullptr;
	m_pCursorSphere = nullptr;

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	/*if (m_pRandomNoiseConstantBuffer)
	{
		delete m_pRandomNoiseConstantBuffer;
		m_pRandomNoiseConstantBuffer = nullptr;
	}
	if (m_pRandomNoise)
	{
		delete m_pRandomNoise;
		m_pRandomNoise = nullptr;
	}*/

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

	m_hMainWindow = nullptr;
	m_hInstance = nullptr;
}

void Renderer::Update(float deltaTime)
{
	_ASSERT(m_pMainCamera);
	_ASSERT(m_pScene);

	m_pMainCamera->UpdateMovements(deltaTime, &m_Keyboard);
	ProcessKeyboardControl(deltaTime);
	ProcessMouseControl();

	m_pPostProcessor->Update();
}

void Renderer::UpdateGUI()
{
	UpdateProfilingUI();
	UpdateRenderOptionUI();

	ImGui::Begin("Scene");
	ImVec2 wsize = ImGui::GetWindowSize();
	ImGui::Image((ImTextureID)(intptr_t)m_pPrevBuffer->pSRV, wsize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
	ImGui::End();
}

void Renderer::Render()
{
	m_pContext->VSSetSamplers(0, (UINT)m_pResourceManager->SamplerStates.size(), m_pResourceManager->SamplerStates.data());
	m_pContext->PSSetSamplers(0, (UINT)m_pResourceManager->SamplerStates.size(), m_pResourceManager->SamplerStates.data());

	PassGBuffer();
	PassShadow(); // SSAO를 구현해야할까?

	m_pScene->GetSkyLUT()->Generate();
	m_pScene->GetAerialLUT()->Generate();

	PassDeferredLighting();

	// PassForward() 구현할 것.
	// 여기에는 PassSSR(반사처리), PassAlpha(투명도 처리), PassDebug를 구현해야 함.
	PassForward();
	PassDebug();

	m_pPostProcessor->Render();

	RenderGUI();

	m_pSwapChain->Present(1, 0);
}

void Renderer::RenderGUI()
{
	SetMainViewport();
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

#ifdef _DEBUG
	//ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
	//for (int i = 0; i < pio.Viewports.Size; i++)
	//{
	//	ImGuiViewport* vp = pio.Viewports[i];
	//	ImDrawData* dd = ImGui::GetPlatformDrawData(vp)->Data[0];
	//	assert(dd != nullptr);  // nullptr이면 문제
	//}
#endif

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		// 생성된 플랫폼 윈도우들에 대해 업데이트 & 렌더까지 처리
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void Renderer::OnResize(int width, int height)
{
	if (!m_pSwapChain || !m_pMainCamera || !m_pScene)
	{
		return;
	}

	// 윈도우가 Minimize 모드에서는 screenWidth/Height가 0.
	if (width <= 0 || height <= 0)
	{
		return;
	
	}
	if (m_ScreenWidth == width && m_ScreenHeight == height)
	{
		return;
	}
	

	m_ScreenWidth = width;
	m_ScreenHeight = height;

#ifdef _DEBUG
	char debugString[256];
	sprintf(debugString, "Resize SwapChain to %d %d\n", width, height);
	OutputDebugStringA(debugString);
#endif 

	// 기존 버퍼 초기화.
	DestroyBuffersForRendering();
	m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

	CreateBuffers();
	m_pMainCamera->SetAspectRatio(GetAspectRatio());
	m_pPostProcessor->Initialize(this,
								 { m_pBackBuffer, m_pFloatBuffer, m_pPrevBuffer, &m_pGBuffer->DepthBuffer },
								 width, height, 4);
	m_pPostProcessor->SetGlobalConstants(m_pScene->GetGlobalConstantBuffer());
}

void Renderer::OnMouseMove(int mouseX, int mouseY)
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
	m_pMainCamera->UpdateDirection(m_Mouse.MouseNDCX, m_Mouse.MouseNDCY);
}

void Renderer::OnMouseClick(bool bLeft, bool bClicked, int mouseX, int mouseY)
{
	if (bLeft)
	{
		m_Mouse.bMouseLeftButton = bClicked;
		if (bClicked && !m_Mouse.bMouseLeftButton)
		{
			m_Mouse.bMouseDragStartFlag = true; // 드래그를 새로 시작하는지 확인.
		}
	}
	else
	{
		m_Mouse.bMouseRightButton = bClicked;
		if (bClicked && !m_Mouse.bMouseRightButton)
		{
			m_Mouse.bMouseDragStartFlag = true; // 드래그를 새로 시작하는지 확인.
		}
	}

	m_Mouse.MouseX = mouseX;
	m_Mouse.MouseY = mouseY;

	m_Mouse.MouseNDCX = mouseX * 2.0f / m_ScreenWidth - 1.0f;
	m_Mouse.MouseNDCY = -mouseY * 2.0f / m_ScreenHeight + 1.0f;
}

void Renderer::OnMouseWheel(WPARAM wheelValue)
{
	m_Mouse.WheelDelta = GET_WHEEL_DELTA_WPARAM(wheelValue);
}

void Renderer::OnKeyboardClick(bool bClicked, WPARAM keyCode)
{
	m_Keyboard.bPressed[keyCode] = bClicked;
	if (!bClicked)
	{
		return;
	}

	if (keyCode == 'F')  // f키 일인칭 시점.
	{
		_ASSERT(m_pMainCamera);
		m_pMainCamera->bUseFirstPersonView = !m_pMainCamera->bUseFirstPersonView;
	}
	if (keyCode == 'P') // 애니메이션 일시중지할 때 사용.
	{
		m_bPauseAnimation = !m_bPauseAnimation;
	}
	if (keyCode == 'Z') // 카메라 설정 화면에 출력.
	{
		_ASSERT(m_pMainCamera);
		m_pMainCamera->PrintView();
	}
	if (keyCode == VK_F1)
	{
		WindowF1Sync();
	}
}

void Renderer::SetConstantBuffers(ID3D11Buffer** ppResources, UINT startSlots, UINT bufferCount, int stage)
{
	_ASSERT(m_pContext);
	_ASSERT(ppResources);
	_ASSERT(bufferCount > 0);
	
	if (stage & PipelineStage_VS)
	{
		m_pContext->VSSetConstantBuffers(startSlots, bufferCount, ppResources);
	}
	if (stage & PipelineStage_GS)
	{
		m_pContext->GSSetConstantBuffers(startSlots, bufferCount, ppResources);
	}
	if (stage & PipelineStage_PS)
	{
		m_pContext->GSSetConstantBuffers(startSlots, bufferCount, ppResources);
	}
	if (stage & PipelineStage_CS)
	{
		m_pContext->CSSetConstantBuffers(startSlots, bufferCount, ppResources);
	}
}

void Renderer::SetShaderResources(ID3D11ShaderResourceView** ppResources, UINT startSlots, UINT bufferCount, int stage)
{
	_ASSERT(m_pContext);
	_ASSERT(ppResources);
	_ASSERT(bufferCount > 0);

	if (stage & PipelineStage_VS)
	{
		m_pContext->VSSetShaderResources(startSlots, bufferCount, ppResources);
	}
	if (stage & PipelineStage_GS)
	{
		m_pContext->GSSetShaderResources(startSlots, bufferCount, ppResources);
	}
	if (stage & PipelineStage_PS)
	{
		m_pContext->PSSetShaderResources(startSlots, bufferCount, ppResources);
	}
	if (stage & PipelineStage_CS)
	{
		m_pContext->CSSetShaderResources(startSlots, bufferCount, ppResources);
	}
}

void Renderer::SetGlobalConsts(ID3D11Buffer** ppGlobalConstsGPU, UINT slot)
{
	// 쉐이더와 일관성 유지 cbuffer GlobalConstants : register(b0).
	m_pContext->VSSetConstantBuffers(slot, 1, ppGlobalConstsGPU);
	m_pContext->PSSetConstantBuffers(slot, 1, ppGlobalConstsGPU);
	m_pContext->GSSetConstantBuffers(slot, 1, ppGlobalConstsGPU);
}

void Renderer::SetViewport(const D3D11_VIEWPORT* pViewports, const UINT NUM_VIEWPORT)
{
	_ASSERT(pViewports);
	_ASSERT(NUM_VIEWPORT > 0);

	m_pContext->RSSetViewports(NUM_VIEWPORT, pViewports);
}

void Renderer::SetPipelineState(const GraphicsPSO* pPSO)
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

void Renderer::SetPipelineState(const ComputePSO* pPSO)
{
	m_pContext->VSSetShader(nullptr, nullptr, 0);
	m_pContext->PSSetShader(nullptr, nullptr, 0);
	m_pContext->HSSetShader(nullptr, nullptr, 0);
	m_pContext->DSSetShader(nullptr, nullptr, 0);
	m_pContext->GSSetShader(nullptr, nullptr, 0);
	m_pContext->CSSetShader(pPSO->pComputeShader, nullptr, 0);
}

Model* Renderer::PickClosest(const DirectX::SimpleMath::Ray* pPickingRay, float* pMinDist)
{
	_ASSERT(m_pScene);

	*pMinDist = 1e5f;
	Model* pMinModel = nullptr;

	for (SIZE_T i = 0, size = m_pScene->RenderObjects.size(); i < size; ++i)
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

void Renderer::InitMainWindow()
{
	_ASSERT(m_hInstance);

	WNDCLASSEX wc = { 0, };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = Renderer::WndProc;
	wc.hInstance = m_hInstance;
	wc.hIcon = nullptr;
	wc.hCursor = nullptr;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"OWL";
	wc.hIconSm = nullptr;

	if (!RegisterClassEx(&wc))
	{
		__debugbreak();
	}

	RECT wr = { 0, 0, (long)m_ScreenWidth, (long)m_ScreenHeight };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	// 창 스타일은 기본(GWL_STYLE), 확장형(GWL_EXSTYLE)이 있음.
	// 기본만 있었는데, 다양한 기능 추가를 위해 확장형을 넣은 것 같음.
	// f11 전체화면과 같은 기능을 구현하기 위해서는 확장형 일부 플래그를 사용해야 함.
	// GWL_STYLE에는 무조건 WS_DLGFRAME가 포함됨(되는 것 같음. 강제로 max or minimize 하지 않는 한)
	// GWL_EXSTYLE는 기본값이 WS_EX_WINDOWEDGE.

	m_hMainWindow = CreateWindowEx(WS_EX_CONTEXTHELP, //WS_EX_WINDOWEDGE,
								   wc.lpszClassName,
								   L"OWL Engine",
								   WS_OVERLAPPEDWINDOW,
								   100,
								   100,
								   wr.right - wr.left,
								   wr.bottom - wr.top,
								   nullptr, nullptr, m_hInstance, this);
	if (!m_hMainWindow)
	{
		__debugbreak();
	}
}

void Renderer::InitD3D()
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

	CreateBuffers();
	SetMainViewport();

	pFactory->Release();
	pAdapter->Release();
}

void Renderer::InitGUI()
{
	_ASSERT(m_hMainWindow);
	_ASSERT(m_pDevice);
	_ASSERT(m_pContext);
	_ASSERT(m_ScreenWidth > 0 && m_ScreenHeight > 0);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)m_ScreenWidth, (float)m_ScreenHeight);
	io.ConfigFlags = ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

	// Setup Platform/Renderer backends
	if (!ImGui_ImplWin32_Init(m_hMainWindow))
	{
		__debugbreak();
	}
	if (!ImGui_ImplDX11_Init(m_pDevice, m_pContext))
	{
		__debugbreak();
	}
}

void Renderer::WindowF1Sync()
{
	// f11을 눌렀을 때, 창없는 전체화면 기능 구현.

	_ASSERT(m_hMainWindow);

	static int s_PrevPosX = 0;
	static int s_PrevPosY = 0;
	static int s_PrevWidth = 0;
	static int s_PrevHeight = 0;
	static DWORD s_PrevWindowStyle = 0;
	static DWORD s_PrevWindowExStyle = 0;

	if (m_bMaximizedWindow) // 현재 전체화면인 상태.
	{
		SetWindowLongPtr(m_hMainWindow, GWL_STYLE, s_PrevWindowStyle);
		SetWindowLongPtr(m_hMainWindow, GWL_EXSTYLE, s_PrevWindowExStyle);

		SetWindowPos(m_hMainWindow,
					 HWND_TOP,
					 s_PrevPosX, s_PrevPosY,
					 s_PrevWidth, s_PrevHeight,
					 SWP_NOZORDER | SWP_FRAMECHANGED);

		s_PrevPosX = 0;
		s_PrevPosY = 0;
		s_PrevWidth = 0;
		s_PrevHeight = 0;
		s_PrevWindowStyle = 0;
		s_PrevWindowExStyle = 0;
	}
	else
	{
		// 이전 정보 저장.

		RECT curWindowRect = {};
		GetWindowRect(m_hMainWindow, &curWindowRect);

		s_PrevPosX = curWindowRect.left;
		s_PrevPosY = curWindowRect.top;
		s_PrevWidth = m_ScreenWidth;
		s_PrevHeight = m_ScreenHeight;
		s_PrevWindowStyle = (DWORD)GetWindowLongPtr(m_hMainWindow, GWL_STYLE);
		s_PrevWindowExStyle = (DWORD)GetWindowLongPtr(m_hMainWindow, GWL_EXSTYLE);


		// 갱신.

		HMONITOR hMonitor = MonitorFromWindow(m_hMainWindow, MONITOR_DEFAULTTONEAREST);
		MONITORINFO monitorInfo = {};
		monitorInfo.cbSize = sizeof(MONITORINFO);
		if (!GetMonitorInfo(hMonitor, &monitorInfo))
		{
			__debugbreak();
		}

		SetWindowLongPtr(m_hMainWindow, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowLongPtr(m_hMainWindow, GWL_EXSTYLE, WS_EX_APPWINDOW);

		SetWindowPos(m_hMainWindow,
					 HWND_TOP,
					 monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
					 monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
					 monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
					 SWP_FRAMECHANGED);
	}

	m_bMaximizedWindow = !m_bMaximizedWindow;
}

void Renderer::CreateBuffers()
{
	HRESULT hr = S_OK;

	ID3D11Texture2D* pBackBufferTexture = nullptr;
	D3D11_TEXTURE2D_DESC desc = {};
	hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBufferTexture));
	BREAK_IF_FAILED(hr);

	if (!m_pBackBuffer)
	{
		m_pBackBuffer = new Texture;
	}
	pBackBufferTexture->GetDesc(&desc);
	m_pBackBuffer->Initialize(m_pDevice, m_pContext, pBackBufferTexture, true);
	RELEASE(pBackBufferTexture);

	// 이전 프레임 저장용.
	m_pBackBuffer->GetTexture2D()->GetDesc(&desc);
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

void Renderer::SetMainViewport()
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

void Renderer::SetComputeShaderBarrier()
{
	// 예제들에서 최대 사용하는 SRV, UAV 갯수가 6개.
	ID3D11ShaderResourceView* ppNullSRVs[6] = { nullptr, };
	ID3D11UnorderedAccessView* ppNullUAVs[6] = { nullptr, };
	m_pContext->CSSetShaderResources(0, 6, ppNullSRVs);
	m_pContext->CSSetUnorderedAccessViews(0, 6, ppNullUAVs, nullptr);
}

void Renderer::DestroyBuffersForRendering()
{
	// swap chain에 사용될 back bufffer와 관련된 모든 버퍼를 초기화.
	m_pBackBuffer->Cleanup();
	m_pGBuffer->Cleanup();
	m_pPrevBuffer->Cleanup();
	m_pFloatBuffer->Cleanup();

	m_pPostProcessor->Cleanup();
}

void Renderer::PassGBuffer()
{
	_ASSERT(m_pGBuffer);
	_ASSERT(m_pScene);

	SetMainViewport();
	SetGlobalConsts(&m_pScene->GetGlobalConstantBuffer()->pBuffer, 0);
	//SetConstantBuffers(&m_pScene->GetGlobalConstantBuffer()->pBuffer, 0, 1, PipelineStage_VS | PipelineStage_GS | PipelineStage_PS);
	//m_pScene->BindGlobalConstantBuffer(0, PipelineStage_VS | PipelineStage_GS | PipelineStage_PS);
	m_pGBuffer->PrepareRender();

	for (SIZE_T i = 0, size = m_pScene->RenderObjects.size(); i < size; ++i)
	{
		Model* const pModel = m_pScene->RenderObjects[i];
		m_pResourceManager->SetPipelineState(pModel->GetGBufferPSO(m_pScene->bDrawAsWire));
		pModel->Render();
	}

	m_pGBuffer->AfterRender();
}

void Renderer::PassShadow()
{
	_ASSERT(m_pScene);

	m_pScene->GetSun()->RenderShadowMap(m_pScene->RenderObjects, nullptr);
	for (SIZE_T i = 0, size = m_pScene->Lights.size(); i < size; ++i)
	{
		m_pScene->Lights[i].RenderShadowMap(m_pScene->RenderObjects, nullptr);
	}
}

void Renderer::PassDeferredLighting()
{
	_ASSERT(m_pScene);

	SetMainViewport();
	m_pResourceManager->SetPipelineState(GraphicsPSOType_DeferredRendering);
	SetGlobalConsts(&m_pScene->GetGlobalConstantBuffer()->pBuffer, 0);
	//SetConstantBuffers(&m_pScene->GetGlobalConstantBuffer()->pBuffer, 0, 1, PipelineStage_VS | PipelineStage_GS | PipelineStage_PS);
	//m_pScene->BindGlobalConstantBuffer(0, PipelineStage_VS | PipelineStage_GS | PipelineStage_PS);

	const float CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_pContext->ClearRenderTargetView(m_pFloatBuffer->pRTV, CLEAR_COLOR);
	m_pContext->OMSetRenderTargets(1, &m_pFloatBuffer->pRTV, nullptr);

	ID3D11ShaderResourceView* ppSRVs[5] = { m_pGBuffer->AlbedoBuffer.pSRV, m_pGBuffer->NormalBuffer.pSRV, m_pGBuffer->PositionBuffer.pSRV, m_pGBuffer->EmissionBuffer.pSRV, m_pGBuffer->ExtraBuffer.pSRV };
	m_pContext->PSSetShaderResources(0, 5, ppSRVs);

	ConstantBuffer* pLightConstantBuffer = m_pScene->GetLightConstantBuffer();
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
	
	Sun* pSun = m_pScene->GetSun();
	memcpy(&pLightConstsData->Lights, &pSun->SunProperty, sizeof(LightProperty));
	pLightConstantBuffer->Upload();
	SetGlobalConsts(&pLightConstantBuffer->pBuffer, 1);
	m_pContext->PSSetShaderResources(7, 1, &pSun->GetShadowMapPtr()->GetCascadeShadowBufferPtr()->pSRV);
	m_pContext->Draw(6, 0);

	for (SIZE_T i = 0, size = m_pScene->Lights.size(); i < size; ++i)
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

void Renderer::PassForward()
{
	PassSky();
	
	// Reflectance


	// Refraction
}

void Renderer::PassSky()
{
	_ASSERT(m_pContext);
	_ASSERT(m_pScene);

	SetMainViewport();
	m_pContext->OMSetRenderTargets(1, &m_pFloatBuffer->pRTV, m_pGBuffer->DepthBuffer.pDSV);

	m_pScene->GetSky()->Render(m_pScene->GetSkyLUT()->GetSkyLUT());
	m_pScene->GetSun()->Render();

	ID3D11RenderTargetView* pNullRTV = nullptr;
	ID3D11DepthStencilView* pNullDSV = nullptr;
	m_pContext->OMSetRenderTargets(1, &pNullRTV, pNullDSV);
}

void Renderer::PassDebug()
{
	_ASSERT(m_pScene);

	SetMainViewport();
	SetGlobalConsts(&m_pScene->GetGlobalConstantBuffer()->pBuffer, 0);
	//SetConstantBuffers(&m_pScene->GetGlobalConstantBuffer()->pBuffer, 0, 1, PipelineStage_VS | PipelineStage_GS | PipelineStage_PS);
	//m_pScene->BindGlobalConstantBuffer(0, PipelineStage_VS | PipelineStage_GS | PipelineStage_PS);
	m_pContext->OMSetRenderTargets(1, &m_pFloatBuffer->pRTV, m_pGBuffer->DepthBuffer.pDSV);

	for (SIZE_T i = 0, size = m_pScene->RenderObjects.size(); i < size; ++i)
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

void Renderer::ProcessKeyboardControl(float deltaTime)
{
	// 키보드 조작에 따른 캐릭터 조작.
	// 만약, 키보드 조작을 따르는 다른 컨트롤이 있다면 추가할 것.

	_ASSERT(m_pScene);
	_ASSERT(m_pScene->pMainController);

	enum
	{
		Idle = 0,
		IdleToWalk,
		Walk,
		WalkToStop
	};

	static int s_State = Idle;
	static int s_FrameCount = 0;

	AnimationData* pAnimationData = &m_pScene->pMainController->CharacterAnimationData;
	const SIZE_T ANIMATION_CLIP_SIZE = pAnimationData->Clips[s_State].Keys[0].size();

	switch (s_State)
	{
	case Idle:
	{
		Vector3 deltaPos = Vector3::One * (pAnimationData->Velocity * deltaTime);
		pAnimationData->Position += deltaPos;

		if (m_Keyboard.bPressed[VK_UP])
		{
			s_State = IdleToWalk;
			s_FrameCount = 0;
			pAnimationData->UpdateVelocity(s_State, s_FrameCount);
		}
		else if (s_FrameCount == ANIMATION_CLIP_SIZE)
		{
			s_FrameCount = 0;
		}

		break;
	}

	case IdleToWalk:
	{
		pAnimationData->UpdateVelocity(s_State, s_FrameCount);

		Vector3 deltaPos = pAnimationData->Direction * (pAnimationData->Velocity * deltaTime);
		pAnimationData->Position += deltaPos;

		if (s_FrameCount == ANIMATION_CLIP_SIZE)
		{
			s_State = Walk;
			s_FrameCount = 0;
			pAnimationData->UpdateVelocity(s_State, s_FrameCount);
		}

		break;
	}

	case Walk:
	{
		if (m_Keyboard.bPressed[VK_RIGHT])
		{
			Quaternion newRot = Quaternion::CreateFromYawPitchRoll(DegreeToRadian(60.0f) * deltaTime * 2.0f, 0.0f, 0.0f);
			pAnimationData->Direction = Vector3::TransformNormal(pAnimationData->Direction, Matrix::CreateFromQuaternion(newRot));
			pAnimationData->Rotation = Quaternion::Concatenate(pAnimationData->Rotation, newRot);
		}
		if (m_Keyboard.bPressed[VK_LEFT])
		{
			Quaternion newRot = Quaternion::CreateFromYawPitchRoll(DegreeToRadian(-60.0f) * deltaTime * 2.0f, 0.0f, 0.0f);
			pAnimationData->Direction = Vector3::TransformNormal(pAnimationData->Direction, Matrix::CreateFromQuaternion(newRot));
			pAnimationData->Rotation = Quaternion::Concatenate(pAnimationData->Rotation, newRot);
		}

		pAnimationData->UpdateVelocity(s_State, s_FrameCount);

		Vector3 deltaPos = pAnimationData->Direction * (pAnimationData->Velocity * deltaTime);
		pAnimationData->Position += deltaPos;

		if (!m_Keyboard.bPressed[VK_UP])
		{
			s_State = WalkToStop;
			s_FrameCount = 0;
		}
		if (s_FrameCount == ANIMATION_CLIP_SIZE)
		{
			s_FrameCount = 0;
			pAnimationData->UpdateVelocity(s_State, s_FrameCount);
		}

		break;
	}

	case WalkToStop:
	{
		pAnimationData->UpdateVelocity(s_State, s_FrameCount);

		Vector3 deltaPos = pAnimationData->Direction * (pAnimationData->Velocity * deltaTime);
		pAnimationData->Position += deltaPos;

		if (s_FrameCount == ANIMATION_CLIP_SIZE)
		{
			s_State = Idle;
			s_FrameCount = 0;
			pAnimationData->UpdateVelocity(s_State, s_FrameCount);
		}

		break;
	}

	default:
		__debugbreak();
		break;
	}

	Matrix newWorld = Matrix::CreateFromQuaternion(pAnimationData->Rotation) * Matrix::CreateTranslation(pAnimationData->Position);
	m_pScene->pMainController->UpdateWorld(newWorld);
	m_pScene->pMainController->UpdateAnimation(s_State, s_FrameCount, deltaTime);

	++s_FrameCount;
}

void Renderer::ProcessMouseControl()
{
	_ASSERT(m_pMainCamera);

	static Model* s_pActiveModel = nullptr;
	static float s_PrevRatio = 0.0f;
	static Vector3 s_PrevPos = Vector3::Zero;
	static Vector3 s_PrevVector = Vector3::Zero;

	// 적용할 회전과 이동 초기화.
	Quaternion dragRotation = Quaternion::CreateFromAxisAngle(Vector3::UnitX, 0.0f);
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


		if (!s_pActiveModel) // 이전 프레임에서 아무 물체도 선택되지 않았을 경우에는 새로 선택.
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

void Renderer::UpdateProfilingUI()
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

void Renderer::UpdateRenderOptionUI()
{
	_ASSERT(m_pMainCamera);
	_ASSERT(m_pScene);
	_ASSERT(m_pPostProcessor);

	ImGui::Begin("RenderOption");

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("DrawOption"))
	{
		ImGui::Checkbox("Use FPV", &m_pMainCamera->bUseFirstPersonView);
		ImGui::Checkbox("Wireframe", &m_pScene->bDrawAsWire);
		ImGui::Checkbox("DrawOBB", &m_pScene->bDrawOBB);
		ImGui::Checkbox("DrawBSphere", &m_pScene->bDrawBS);
		ImGui::TreePop();
	}

	m_pPostProcessor->UpdateGUI();

	ImGui::End();
}
