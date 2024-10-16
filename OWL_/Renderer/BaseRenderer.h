#pragma once

#include "../Util/KnM.h"

struct MeshInfo;
class Camera;
class ComputePSO;
class GBuffer;
class GraphicsPSO;
class Model;
class Scene;
class Timer;
class ResourceManager;
class PostProcessor;
class Texture;

class BaseRenderer
{
public:
	BaseRenderer();
	virtual ~BaseRenderer();

	virtual void Initialize(Scene* const pScene);
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
	inline PostProcessor* GetPostProcessor() { return m_pPostProcessor; }
	inline Timer* GetTimer() { return m_pTimer; }
	inline Camera* GetCamera() { return m_pMainCamera; }

	inline Model* GetPickedModel() { return m_pPickedModel; }
	inline Keyboard* GetKeyboard() { return &m_Keyboard; }
	inline Mouse* GetMouse() { return &m_Mouse; }

	inline void SetPickedModel(Model* const pModel) { m_pPickedModel = pModel; }
	void SetGlobalConsts(ID3D11Buffer** ppGlobalConstsGPU, UINT slot);
	void SetViewport(const D3D11_VIEWPORT* pViewports, const UINT NUM_VIEWPORT);
	void SetPipelineState(const GraphicsPSO* pPSO);
	void SetPipelineState(const ComputePSO* pPSO);

	Model* PickClosest(const DirectX::SimpleMath::Ray* pPickingRay, float* pMinDist);
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
	PostProcessor* m_pPostProcessor = nullptr;

	Texture* m_pBackBuffer = nullptr;
	Texture* m_pFloatBuffer = nullptr;
	Texture* m_pPrevBuffer = nullptr;
	GBuffer* m_pGBuffer = nullptr;

	Camera* m_pMainCamera = nullptr;
	Keyboard m_Keyboard;
	Mouse m_Mouse;

	bool m_bPauseAnimation = false;

	Model* m_pPickedModel = nullptr; // 마우스 선택용.
	Model* m_pCursorSphere = nullptr; // 드래그 표시용.

	// for debugging.
	Timer* m_pTimer = nullptr;
	std::vector<float> m_DeltaTimeData;
	std::vector<float> m_FrameRateData;

	// DO NOT release directly.
	Scene* m_pScene = nullptr;
};
