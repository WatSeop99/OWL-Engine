﻿#pragma once

#include "Camera.h"
#include "GraphicsCommon.h"
#include "../Geometry/MeshInfo.h"
#include "../Geometry/Model.h"
#include "PostProcessor.h"
#include "Scene.h"
#include "Timer.h"

namespace Core
{
	using DirectX::BoundingSphere;
	using DirectX::SimpleMath::Quaternion;
	using DirectX::SimpleMath::Ray;
	using DirectX::SimpleMath::Vector3;
	using std::shared_ptr;
	using std::vector;
	using std::wstring;

	class BaseRenderer
	{
	public:
		BaseRenderer();
		virtual ~BaseRenderer();

		int Run();

		virtual void Initialize();
		virtual void InitScene();

		virtual void UpdateGUI();
		virtual void Update(float deltaTime);

		// virtual void RenderDepthOnly();
		// virtual void RenderShadowMaps();
		// virtual void RenderOpaqueObjects();
		//
		/*virtual void RenderGBuffer();
		virtual void RenderLights();*/
		//
		// virtual void RenderMirror(); // forward shading 처리.
		virtual void RenderGUI();
		virtual void Render();

		virtual void OnMouseMove(int mouseX, int mouseY);
		virtual void OnMouseClick(int mouseX, int mouseY);
		virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		inline float GetAspectRatio() const { return (float)m_ScreenWidth / (float)m_ScreenHeight; }
		
		void SetGlobalConsts(ID3D11Buffer** ppGlobalConstsGPU, UINT slot);
		void SetPipelineState(const Graphics::GraphicsPSO& PSO);
		void SetPipelineState(const Graphics::ComputePSO& PSO);
		
		Geometry::Model* PickClosest(const Ray& PICKNG_RAY, float* pMinDist);
		void ProcessMouseControl();

	protected:
		void initMainWindow();
		void initDirect3D();
		void initGUI();

		void createBuffers();

		void setMainViewport();
		void setComputeShaderBarrier();

		void destroyBuffersForRendering();

	protected:
		UINT m_ScreenWidth = 1280;
		UINT m_ScreenHeight = 720;
		HWND m_hMainWindow = nullptr;
		bool m_bUseMSAA = false;
		UINT m_NumQualityLevels = 0;

		D3D_FEATURE_LEVEL m_FeatureLevel;
		DXGI_ADAPTER_DESC2 m_AdapterDesc = { 0, };
		DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		D3D11_VIEWPORT m_ScreenViewport = { 0, };

		ID3D11Device* m_pDevice = nullptr;
		ID3D11Device5* m_pDevice5 = nullptr;
		ID3D11DeviceContext* m_pContext = nullptr;
		ID3D11DeviceContext4* m_pContext4 = nullptr;
		IDXGISwapChain* m_pSwapChain = nullptr;
		IDXGISwapChain4* m_pSwapChain4 = nullptr;

		ID3D11Texture2D* m_pBackBuffer = nullptr;
		ID3D11RenderTargetView* m_pBackBufferRTV = nullptr;

		// Deferred shading을 위한 시험용 멤버변수.
		////////////////////////////////////////////////////
		
		/*GBuffer m_GBuffer;
		LightManager m_LightManager;*/

		////////////////////////////////////////////////////

		// 삼각형 레스터화 -> float(MSAA) -> resolved(No MSAA)
		// -> 후처리(블룸, 톤매핑) -> backBuffer(최종 SwapChain Present)
		Graphics::Texture2D m_FloatBuffer;
		Graphics::Texture2D m_ResolvedBuffer;
		Graphics::Texture2D m_PrevBuffer; // 간단한 모션 블러 효과를 위함.
		
		// 레벨.
		Scene m_Scene;

		// 렌더링 -> PostEffects -> PostProcess
		PostProcessor m_PostProcessor;

		// 시점을 결정하는 카메라 클래스 추가
		Camera m_Camera;
		bool m_pbKeyPressed[256] = { false, };

		bool m_bLeftButton = false;
		bool m_bRightButton = false;
		bool m_bDragStartFlag = false;

		// 마우스 커서 위치 저장 (Picking에 사용)
		float m_MouseNDCX = 0.0f;
		float m_MouseNDCY = 0.0f;
		float m_WheelDelta = 0.0f;
		int m_MouseX = -1;
		int m_MouseY = -1;

		bool m_bPauseAnimation = false;

		Geometry::Model* m_pPickedModel = nullptr; // 마우스 선택용.
		Geometry::Model* m_pCursorSphere = nullptr; // 드래그 표시용.

		// for debugging.
		Timer* m_pTimer = nullptr;
	};
}
