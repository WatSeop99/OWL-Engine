#pragma once

#include "Camera.h"
#include "ConstantBuffers.h"
#include "GraphicsCommon.h"
#include "Light.h"
#include "../Geometry/MeshInfo.h"
#include "../Geometry/Model.h"
#include "PostProcessor.h"
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
		inline float GetAspectRatio() const { return (float)m_ScreenWidth / (float)m_ScreenHeight; }

		virtual void Initialize();
		virtual void InitScene();
		void InitCubemaps(std::wstring&& basePath, std::wstring&& envFileName, std::wstring&& specularFileName, std::wstring&& irradianceFileName, std::wstring&& brdfFileName);

		virtual void UpdateGUI();
		virtual void UpdateLights(float deltaTime);
		void UpdateGlobalConstants(const float DELTA_TIME, const Vector3& EYE_WORLD, const Matrix& VIEW, const Matrix& PROJECTION, const Matrix& REFLECTION = Matrix());
		virtual void Update(float deltaTime);

		virtual void RenderDepthOnly();
		virtual void RenderShadowMaps();
		virtual void RenderOpaqueObjects();
		//
		/*virtual void RenderGBuffer();
		virtual void RenderLights();*/
		//
		virtual void RenderMirror(); // forward shading 처리.
		virtual void RenderGUI();
		virtual void Render();

		virtual void OnMouseMove(int mouseX, int mouseY);
		virtual void OnMouseClick(int mouseX, int mouseY);
		virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		
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
		void createDepthBuffers();

		void setMainViewport();
		// void setShadowViewport();
		void setComputeShaderBarrier();

		void destroyBuffersForRendering();

	protected:
		UINT m_ScreenWidth;
		UINT m_ScreenHeight;
		HWND m_hMainWindow;
		bool m_bUseMSAA;
		UINT m_NumQualityLevels;
		bool m_bDrawAsWire;
		bool m_bDrawOBB; // Draw Object Oriented Bounding Box
		bool m_bDrawBS;  // Draw Bounding Sphere

		D3D_FEATURE_LEVEL m_FeatureLevel;
		DXGI_ADAPTER_DESC2 m_AdapterDesc;
		DXGI_FORMAT m_BackBufferFormat;
		D3D11_VIEWPORT m_ScreenViewport;

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
		Texture2D m_FloatBuffer;
		Texture2D m_ResolvedBuffer;
		Texture2D m_PrevBuffer; // 간단한 모션 블러 효과를 위함.

		// Depth buffer 관련.
		ID3D11DepthStencilView* m_pDefaultDSV = nullptr;
		Texture2D m_DepthOnlyBuffer; // No MSAA

		// Light.
		Light m_pLights[MAX_LIGHTS];

		// 시점을 결정하는 카메라 클래스 추가
		Camera m_Camera;
		bool m_pbKeyPressed[256] = { false, };

		bool m_bLeftButton;
		bool m_bRightButton;
		bool m_bDragStartFlag;

		// 마우스 커서 위치 저장 (Picking에 사용)
		float m_MouseNDCX;
		float m_MouseNDCY;
		float m_WheelDelta;
		int m_MouseX;
		int m_MouseY;

		// 렌더링 -> PostEffects -> PostProcess
		PostProcessor m_PostProcessor;

		// 다양한 Pass들을 더 간단히 구현하기 위해 ConstBuffer들 분리
		GlobalConstants m_GlobalConstsCPU;
		ID3D11Buffer* m_pGlobalConstsGPU = nullptr;

		GlobalConstants m_ReflectGlobalConstsCPU;
		ID3D11Buffer* m_pReflectGlobalConstsGPU = nullptr;

		LightConstants m_LightConstantsCPU;
		ID3D11Buffer* m_pLightConstantsGPU = nullptr;

		// 공통으로 사용하는 텍스쳐들.
		ID3D11ShaderResourceView* m_pEnvSRV = nullptr;
		ID3D11ShaderResourceView* m_pIrradianceSRV = nullptr;
		ID3D11ShaderResourceView* m_pSpecularSRV = nullptr;
		ID3D11ShaderResourceView* m_pBrdfSRV = nullptr;

		bool m_bPauseAnimation;

		// 여러 예제들 공용.
		Geometry::Model* m_pScreenSquare = nullptr; // PostEffect에 사용
		Geometry::Model* m_pSkybox = nullptr;
		Geometry::Model* m_pPickedModel = nullptr;
		Geometry::Model* m_ppLightSpheres[MAX_LIGHTS] = { nullptr, };
		Geometry::Model* m_pCursorSphere = nullptr;
		Geometry::Model* m_pMirror = nullptr; // 거울은 별도로 그림
		DirectX::SimpleMath::Plane m_MirrorPlane;
		float m_MirrorAlpha; // Opacity

		// 거울이 아닌 물체들의 리스트. (for문으로 그리기 위함)
		std::vector<Geometry::Model*> m_pBasicList;

		// for debugging.
		Timer* m_pTimer = nullptr;
	};
}
