#pragma once

#include "ConstantDataType.h"
#include "../Renderer/ConstantBuffer.h"
#include "../Renderer/GBuffer.h"
#include "Light.h"

class BaseRenderer;
class Camera;
class Mesh;
class Model;

class Scene
{
public:
	Scene(Camera& camera, Texture2D& floatBuffer, Texture2D& resolvedBuffer, UINT width = 1280, UINT height = 720) :
		m_Camera(camera),
		m_FloatBuffer(floatBuffer),
		m_ResolvedBuffer(resolvedBuffer),
		m_ScreenWidth(width),
		m_ScreenHeight(height),
		m_GBuffer(floatBuffer, width, height)
	{}
	~Scene() { Cleanup(); }

	void Initialize(BaseRenderer* pRenderer);

	void Update(const float DELTA_TIME);

	void Render();

	void Cleanup();
	void ResetBuffers(const bool bUSE_MSAA = false, const UINT NUM_QUALITY_LEVELS = 0);

	inline void SetScreenWidth(const UINT WIDTH) { m_ScreenWidth = WIDTH; m_GBuffer.SetScreenWidth(WIDTH); }
	inline void SetScreenHeight(const UINT HEIGHT) { m_ScreenHeight = HEIGHT; m_GBuffer.SetScreenHeight(HEIGHT); }

	inline bool SupportDeferred() const { return m_GBuffer.bIsEnabled; }
	inline GlobalConstants* GetGlobalConstantsCPU() { return (GlobalConstants*)m_GlobalConstants.pSystemMem; }
	inline ID3D11Buffer* GetGlobalConstantsGPU() const { return m_GlobalConstants.pBuffer; }
	inline ID3D11ShaderResourceView* GetDepthOnlyBufferSRV() const { return m_DepthOnlyBuffer.pSRV; }
	inline Model* GetMirror() { return m_pMirror; }

	void LoadScene()
	{
		// initCubemaps(pDevice, L"./Assets/Textures/Cubemaps/HDRI/", L"SampleEnvHDR.dds", L"SampleSpecularHDR.dds", L"SampleDiffuseHDR.dds", L"SampleBrdf.dds"); 
		initCubemaps(L"./Assets/Textures/Cubemaps/HDRI/", L"clear_pureskyEnvHDR.dds", L"clear_pureskySpecularHDR.dds", L"clear_pureskyDiffuseHDR.dds", L"clear_pureskyBrdf.dds");
	}
	void SaveScene() {}

protected:
	void initCubemaps(std::wstring&& basePath, std::wstring&& envFileName, std::wstring&& specularFileName, std::wstring&& irradianceFileName, std::wstring&& brdfFileName);

	void createBuffers(const bool bUSE_MSAA = false, const UINT NUM_QUALITY_LEVELS = 0);
	void createDepthBuffers(const bool bUSE_MSAA, const UINT NUM_QUALITY_LEVELS);

	void updateLights(const float DELTA_TIME);
	void updateGlobalConstants(const float DELTA_TIME);

	void renderDepthOnly();
	void renderShadowMaps();
	void renderOpaqueObjects();
	void renderGBuffer();
	void renderDeferredLighting();
	void renderOptions();
	void renderMirror();

	void setPipelineState(const GraphicsPSO& PSO);
	void setScreenViewport();
	void setGlobalConstants(ID3D11Buffer** ppGlobalConstants, UINT slot);

public:
	std::vector<Model*> pRenderObjects; // opaque.
	std::vector<Light> pLights;

	bool bDrawAsWire = false;
	bool bDrawOBB = false;
	bool bDrawBS = false;

	float MirrorAlpha = 1.0f; // Opacity

private:
	BaseRenderer* m_pRenderer = nullptr;

	UINT m_ScreenWidth;
	UINT m_ScreenHeight;
	Mesh* m_pScreenMesh = nullptr;
	Camera& m_Camera;
	Texture2D& m_FloatBuffer;
	Texture2D& m_ResolvedBuffer;

	// g-buffer.
	GBuffer m_GBuffer;

	// depth buffer 관련.
	ID3D11DepthStencilView* m_pDefaultDSV = nullptr;
	Texture2D m_DepthOnlyBuffer; // No MSAA

	// 렌더링을 위한 여러 상수 퍼버들.
	ConstantBuffer m_GlobalConstants;
	ConstantBuffer m_ReflectionGlobalConstants;
	ConstantBuffer m_LightConstants;

	// 공통으로 사용하는 환경맵 리소스들.
	ID3D11ShaderResourceView* m_pEnvSRV = nullptr;
	ID3D11ShaderResourceView* m_pIrradianceSRV = nullptr;
	ID3D11ShaderResourceView* m_pSpecularSRV = nullptr;
	ID3D11ShaderResourceView* m_pBrdfSRV = nullptr;

	// 여러 예제들 공용.
	Model* m_pSkybox = nullptr;
	Model* m_pGround = nullptr;
	std::vector<Model*> m_ppLightSpheres;
	Model* m_pMirror = nullptr; // 거울은 별도로 그림
	DirectX::SimpleMath::Plane m_MirrorPlane;

};
