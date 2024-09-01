#pragma once

#include "ConstantDataType.h"
#include "../Renderer/ConstantBuffer.h"
#include "../Renderer/GBuffer.h"
#include "Light.h"

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

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

	void Update(ID3D11DeviceContext* pContext, const float DELTA_TIME);

	void Render(ID3D11DeviceContext* pContext);

	void Cleanup();
	void ResetBuffers(ID3D11Device* pDevice, const bool bUSE_MSAA = false, const UINT NUM_QUALITY_LEVELS = 0);

	inline void SetScreenWidth(const UINT WIDTH) { m_ScreenWidth = WIDTH; m_GBuffer.SetScreenWidth(WIDTH); }
	inline void SetScreenHeight(const UINT HEIGHT) { m_ScreenHeight = HEIGHT; m_GBuffer.SetScreenHeight(HEIGHT); }

	inline bool SupportDeferred() const { return m_GBuffer.bIsEnabled; }
	inline GlobalConstants* GetGlobalConstantsCPU() { return (GlobalConstants*)m_GlobalConstants.pSystemMem; }
	inline ID3D11Buffer* GetGlobalConstantsGPU() const { return m_GlobalConstants.pBuffer; }
	inline ID3D11ShaderResourceView* GetDepthOnlyBufferSRV() const { return m_DepthOnlyBuffer.pSRV; }
	inline Model* GetMirror() { return m_pMirror; }

	void LoadScene(ID3D11Device* pDevice)
	{
		// initCubemaps(pDevice, L"./Assets/Textures/Cubemaps/HDRI/", L"SampleEnvHDR.dds", L"SampleSpecularHDR.dds", L"SampleDiffuseHDR.dds", L"SampleBrdf.dds"); 
		initCubemaps(pDevice, L"./Assets/Textures/Cubemaps/HDRI/", L"clear_pureskyEnvHDR.dds", L"clear_pureskySpecularHDR.dds", L"clear_pureskyDiffuseHDR.dds", L"clear_pureskyBrdf.dds");
	}
	void SaveScene() {}

protected:
	void initCubemaps(ID3D11Device* pDevice, std::wstring&& basePath, std::wstring&& envFileName, std::wstring&& specularFileName, std::wstring&& irradianceFileName, std::wstring&& brdfFileName);

	void createBuffers(ID3D11Device* pDevice, const bool bUSE_MSAA = false, const UINT NUM_QUALITY_LEVELS = 0);
	void createDepthBuffers(ID3D11Device* pDevice, const bool bUSE_MSAA, const UINT NUM_QUALITY_LEVELS);

	void updateLights(ID3D11DeviceContext* pContext, const float DELTA_TIME);
	void updateGlobalConstants(ID3D11DeviceContext* pContext, const float DELTA_TIME);

	void renderDepthOnly(ID3D11DeviceContext* pContext);
	void renderShadowMaps(ID3D11DeviceContext* pContext);
	void renderOpaqueObjects(ID3D11DeviceContext* pContext);
	void renderGBuffer(ID3D11DeviceContext* pContext);
	void renderDeferredLighting(ID3D11DeviceContext* pContext);
	void renderOptions(ID3D11DeviceContext* pContext);
	void renderMirror(ID3D11DeviceContext* pContext);

	void setPipelineState(ID3D11DeviceContext* pContext, const GraphicsPSO& PSO);
	void setScreenViewport(ID3D11DeviceContext* pContext);
	void setGlobalConstants(ID3D11DeviceContext* pContext, ID3D11Buffer** ppGlobalConstants, UINT slot);

public:
	std::vector<Model*> pRenderObjects; // opaque.
	std::vector<Light> pLights;

	bool bDrawAsWire = false;
	bool bDrawOBB = false;
	bool bDrawBS = false;

	float MirrorAlpha = 1.0f; // Opacity

private:
	UINT m_ScreenWidth;
	UINT m_ScreenHeight;
	Mesh* m_pScreenMesh = nullptr;
	Camera& m_Camera;
	Texture2D& m_FloatBuffer;
	Texture2D& m_ResolvedBuffer;

	// g-buffer.
	GBuffer m_GBuffer;

	// depth buffer ����.
	ID3D11DepthStencilView* m_pDefaultDSV = nullptr;
	Texture2D m_DepthOnlyBuffer; // No MSAA

	// �������� ���� ���� ��� �۹���.
	ConstantBuffer m_GlobalConstants;
	ConstantBuffer m_ReflectionGlobalConstants;
	ConstantBuffer m_LightConstants;

	// �������� ����ϴ� ȯ��� ���ҽ���.
	ID3D11ShaderResourceView* m_pEnvSRV = nullptr;
	ID3D11ShaderResourceView* m_pIrradianceSRV = nullptr;
	ID3D11ShaderResourceView* m_pSpecularSRV = nullptr;
	ID3D11ShaderResourceView* m_pBrdfSRV = nullptr;

	// ���� ������ ����.
	Model* m_pSkybox = nullptr;
	Model* m_pGround = nullptr;
	std::vector<Model*> m_ppLightSpheres;
	Model* m_pMirror = nullptr; // �ſ��� ������ �׸�
	DirectX::SimpleMath::Plane m_MirrorPlane;

};
