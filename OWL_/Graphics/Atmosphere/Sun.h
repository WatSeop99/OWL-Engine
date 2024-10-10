#pragma once

#include "../ConstantDataType.h"

ALIGN(16) struct SunVSConstants
{
	Matrix WorldViewProjection;
	float SunTheta;
	float EyeHeight;
	float pad[2];
};
ALIGN(16) struct SunPSConstants
{
	Vector3 Radiance;
	float pad;
};

class BaseRenderer;
class Camera;
class ConstantBuffer;
class ShadowMap;
class Texture;

class Sun
{
public:
	Sun() = default;
	~Sun() { Cleanup(); };

	void Initialize(BaseRenderer* pRenderer, Camera* pMainCamera);

	void Update();

	void Render();
	void RenderShadowMap(std::vector<Model*>& basicList, Model* pMirror);

	void ResetSunDisk();

	void Cleanup();

	inline ShadowMap* GetShadowMapPtr() { return m_pSunShadowMap; }
	inline float GetWorldScale() { return m_WorldScale; }

	void SetCamera(const Vector3* const pEye, const Matrix* const pViewProj);
	inline void SetWorldScale(const float WORLD_SCALE) { m_WorldScale = WORLD_SCALE; }
	inline void SetAtmosphere(ConstantBuffer* const pAtmos) { m_pAtmosphereConstantBuffer = pAtmos; }
	inline void SetTransmittanceLUT(Texture* const pTransmittanceLUT) { m_pTransmittanceLUT = pTransmittanceLUT; }

protected:
	void createSunMesh(const int SEG_COUNT);
	void createConstantBuffers();

public:
	int SunDiskSegments = 32;

	float SunIntensity = 10.0f;

	Vector2 SunAngle = Vector2(0.0f, 11.6f);
	Vector3 SunColor = Vector3(1.0f);
	Matrix SunWorld;
	Matrix SunViewProjection;

	LightProperty SunProperty;

private:
	ID3D11Buffer* m_pSunDiskBuffer = nullptr;
	std::vector<Vector2> m_SunDiskVertices;

	ConstantBuffer* m_pSunVSConstantBuffer = nullptr;
	ConstantBuffer* m_pSunPSConstantBuffer = nullptr;

	Camera* m_pSunCamera = nullptr;
	ShadowMap* m_pSunShadowMap = nullptr;

	// Datas.
	float m_WorldScale = 0.0f;
	Vector3 m_CameraEye;
	Matrix m_CameraViewProjection;

	// DO NOT release directly.
	BaseRenderer* m_pRenderer = nullptr;
	Camera* m_pMainCamera = nullptr;
	ConstantBuffer* m_pAtmosphereConstantBuffer = nullptr;
	Texture* m_pTransmittanceLUT = nullptr;
};
