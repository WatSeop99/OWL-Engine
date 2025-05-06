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

class Renderer final
{
private:
	static LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
	Renderer() = default;
	~Renderer() { Cleanup(); }

	bool Initialize(HINSTANCE hInstance, Scene* const pScene);
	bool InitScene();

	void Cleanup();

	void UpdateGUI();
	void Update(float deltaTime);

	void RenderGUI();
	void Render();

	void OnResize(int width, int height);
	void OnMouseMove(int mouseX, int mouseY);
	void OnMouseClick(bool bLeft, bool bClicked, int mouseX, int mouseY);
	void OnMouseWheel(WPARAM wheelValue);
	void OnKeyboardClick(bool bClicked, WPARAM keyCode);

	inline HWND GetWindowHandle() { return m_hMainWindow; }
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
	void ProcessKeyboardControl(float deltaTime);
	void ProcessMouseControl();

private:
	void InitMainWindow();
	void InitD3D();
	void InitGUI();

	void WindowF1Sync();

	void CreateBuffers();

	void SetMainViewport();
	void SetComputeShaderBarrier();

	void DestroyBuffersForRendering();

	void PassGBuffer();
	void PassShadow();
	void PassDeferredLighting();
	void PassSky();
	void PassDebug();

private:
	HINSTANCE m_hInstance = nullptr;
	HWND m_hMainWindow = nullptr;
	int m_ScreenWidth = 1920;
	int m_ScreenHeight = 1080;
	UINT m_NumQualityLevels = 0;

	D3D_FEATURE_LEVEL m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;
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
	Keyboard m_Keyboard = {};
	Mouse m_Mouse = {};

	bool m_bPauseAnimation = false;
	bool m_bMaximizedWindow = false;

	Model* m_pPickedModel = nullptr; // 마우스 선택용.
	Model* m_pCursorSphere = nullptr; // 드래그 표시용.

	// for debugging.
	Timer* m_pTimer = nullptr;
	std::vector<float> m_DeltaTimeData;
	std::vector<float> m_FrameRateData;

	// DO NOT release directly.
	Scene* m_pScene = nullptr;
};
