#pragma once

#include "Atmosphere/AtmosphereProperty.h"
#include "ConstantDataType.h"
#include "../Renderer/ConstantBuffer.h"
#include "../Renderer/GBuffer.h"
#include "Light.h"
#include "../Renderer/Texture.h"

class AerialLUT;
class BaseRenderer;
class Camera;
class Mesh;
class Model;
class MultiScatteringLUT;
class Sky;
class SkyLUT;
class Sun;
class TransmittanceLUT;

class Scene
{
public:
	Scene(Camera& camera, Texture* pFloatBuffer, Texture* pResolvedBuffer, UINT width = 1280, UINT height = 720) :
		m_Camera(camera),
		m_pFloatBuffer(pFloatBuffer),
		m_pResolvedBuffer(pResolvedBuffer),
		m_ScreenWidth(width),
		m_ScreenHeight(height)
	{}
	~Scene() { Cleanup(); }

	void Initialize(BaseRenderer* pRenderer);

	void Update(const float DELTA_TIME);

	void Cleanup();

	inline ConstantBuffer* GetGlobalConstantBufferPtr() { return &m_GlobalConstants; }
	inline ConstantBuffer* GetLightConstantBufferPtr() { return &m_LightConstants; }

	inline AerialLUT* GetAerialLUTPtr() { return m_pAerialLUT; }
	inline SkyLUT* GetSkyLUTPtr() { return m_pSkyLUT; }
	inline Sky* GetSkyPtr() { return m_pSky; }
	inline Sun* GetSunPtr() { return m_pSun; }

	inline GlobalConstants* GetGlobalConstantsCPU() { return (GlobalConstants*)m_GlobalConstants.pSystemMem; }
	inline ID3D11Buffer* GetGlobalConstantsGPU() const { return m_GlobalConstants.pBuffer; }
	inline Model* GetMirror() { return m_pMirror; }

	void LoadScene()
	{
		// initCubemaps(pDevice, L"./Assets/Textures/Cubemaps/HDRI/", L"SampleEnvHDR.dds", L"SampleSpecularHDR.dds", L"SampleDiffuseHDR.dds", L"SampleBrdf.dds"); 
		initCubemaps(L"./Assets/Textures/Cubemaps/HDRI/", L"clear_pureskyEnvHDR.dds", L"clear_pureskySpecularHDR.dds", L"clear_pureskyDiffuseHDR.dds", L"clear_pureskyBrdf.dds");
	}
	void SaveScene() {}

protected:
	void initCubemaps(std::wstring&& basePath, std::wstring&& envFileName, std::wstring&& specularFileName, std::wstring&& irradianceFileName, std::wstring&& brdfFileName);

	void updateLights(const float DELTA_TIME);
	void updateGlobalConstants(const float DELTA_TIME);

public:
	std::vector<Model*> RenderObjects; // opaque.
	std::vector<Light> Lights;

	bool bDrawAsWire = false;
	bool bDrawOBB = false;
	bool bDrawBS = false;

	float MirrorAlpha = 1.0f; // Opacity

private:
	BaseRenderer* m_pRenderer = nullptr;

	UINT m_ScreenWidth;
	UINT m_ScreenHeight;
	Camera& m_Camera;
	Texture* m_pFloatBuffer = nullptr;
	Texture* m_pResolvedBuffer = nullptr;

	// 렌더링을 위한 여러 상수 퍼버들.
	ConstantBuffer m_GlobalConstants;
	ConstantBuffer m_ReflectionGlobalConstants;
	ConstantBuffer m_LightConstants;

	// 공통으로 사용하는 환경맵 리소스들.
	Texture* m_pEnv = nullptr;
	Texture* m_pIrradiance = nullptr;
	Texture* m_pSpecular = nullptr;
	Texture* m_pBRDF = nullptr;
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

	// temp atmosphere data
	LightProperty m_SunProperty;
	Camera m_SunCamera;
	ShadowMap m_ShadowMap = ShadowMap(2560, 2560);
	AtmosphereProperty m_AtmosphereProperty;
	ConstantBuffer* m_pAtmosphereConstantBuffer = nullptr;
	Sky* m_pSky = nullptr;
	SkyLUT* m_pSkyLUT = nullptr;
	AerialLUT* m_pAerialLUT = nullptr;
	TransmittanceLUT* m_pTransmittanceLUT = nullptr;
	MultiScatteringLUT* m_pMultiScatteringLUT = nullptr;
	Sun* m_pSun = nullptr;
};
