#pragma once

#include "../Graphics/Camera.h"
#include "../Util/KnM.h"
#include "ResourceManager.h"
#include "../Renderer/PostProcessor.h"
#include "../Graphics/Scene.h"
#include "Texture.h"

using DirectX::BoundingSphere;
using DirectX::SimpleMath::Quaternion;
using DirectX::SimpleMath::Ray;
using DirectX::SimpleMath::Vector3;

struct MeshInfo;
class GBuffer;
class Model;
class Timer;

class BaseRenderer
{
public:
	BaseRenderer();
	virtual ~BaseRenderer();

	virtual void Initialize();
	virtual void InitScene();

	virtual void UpdateGUI();
	virtual void Update(float deltaTime);

	virtual void RenderGUI();
	virtual void Render();

	virtual void OnMouseMove(int mouseX, int mouseY);
	virtual void OnMouseClick(int mouseX, int mouseY);
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	inline float GetAspectRatio() { return (float)m_ScreenWidth / (float)m_ScreenHeight; }
	inline ID3D11Device* GetDevice() { return m_pDevice; }
	inline ID3D11DeviceContext* GetDeviceContext() { return m_pContext; }
	inline ResourceManager* GetResourceManager() { return m_pResourceManager; }
	inline PostProcessor* GetPostProcessor() { return &m_PostProcessor; }
	inline Timer* GetTimer() { return m_pTimer; }
	inline Camera* GetCamera() { return &m_Camera; }
	inline Scene* GetScene() { return &m_Scene; }

	inline Model* GetPickedModel() { return m_pPickedModel; }
	inline Keyboard* GetKeyboard() { return &m_Keyboard; }
	inline Mouse* GetMouse() { return &m_Mouse; }

	inline void SetPickedModel(Model* const pModel) { m_pPickedModel = pModel; }
	void SetGlobalConsts(ID3D11Buffer** ppGlobalConstsGPU, UINT slot);
	void SetViewport(const D3D11_VIEWPORT* pViewports, const UINT NUM_VIEWPORT);
	void SetPipelineState(const GraphicsPSO& PSO);
	void SetPipelineState(const ComputePSO& PSO);

	Model* PickClosest(const Ray& PICKNG_RAY, float* pMinDist);
	void ProcessMouseControl();

protected:
	void initMainWindow();
	void initDirect3D();
	void initGUI();

	void createBuffers();

	void setMainViewport();
	void setComputeShaderBarrier();

	void destroyBuffersForRendering();

	void passGBuffer();
	void passShadow();
	void passDeferredLighting();
	void passSky();
	void passDebug();

protected:
	UINT m_ScreenWidth = 1920;
	UINT m_ScreenHeight = 1080;
	HWND m_hMainWindow = nullptr;
	UINT m_NumQualityLevels = 0;

	D3D_FEATURE_LEVEL m_FeatureLevel;
	DXGI_ADAPTER_DESC2 m_AdapterDesc = { 0, };
	DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D11_VIEWPORT m_ScreenViewport = { 0, };

	ID3D11Device5* m_pDevice = nullptr;
	ID3D11DeviceContext4* m_pContext = nullptr;
	IDXGISwapChain4* m_pSwapChain = nullptr;

	ResourceManager* m_pResourceManager = nullptr;

	Texture* m_pBackBuffer = nullptr;
	Texture m_FloatBuffer;
	Texture m_PrevBuffer; // 간단한 모션 블러 효과를 위함.
	GBuffer* m_pGBuffer = nullptr;

	// 레벨.
	Scene m_Scene;

	// 렌더링 -> PostEffects -> PostProcess
	PostProcessor m_PostProcessor;

	// 시점을 결정하는 카메라 클래스 추가
	Camera m_Camera;
	Keyboard m_Keyboard;
	Mouse m_Mouse;

	bool m_bPauseAnimation = false;

	Model* m_pPickedModel = nullptr; // 마우스 선택용.
	Model* m_pCursorSphere = nullptr; // 드래그 표시용.

	// for debugging.
	Timer* m_pTimer = nullptr;
};
