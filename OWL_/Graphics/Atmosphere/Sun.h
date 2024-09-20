#pragma once

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
class ConstantBuffer;
class Texture;

class Sun
{
public:
	Sun() = default;
	~Sun() { Cleanup(); };

	void Initialize(BaseRenderer* pRenderer);

	void Update();

	void Render();

	void ResetSunDisk();

	void Cleanup();

	void SetCamera(const Vector3* const pEye, const Matrix* const pViewProj);
	inline void SetWorldScale(const float WORLD_SCALE) { m_WorldScale = WORLD_SCALE; }
	inline void SetAtmosphere(ConstantBuffer* const pAtmos) { m_pAtmosphereConstantBuffer = pAtmos; }
	inline void SetTransmittanceLUT(Texture* const pTransmittanceLUT) { m_pTransmittanceLUT = pTransmittanceLUT; }

protected:
	void createSunMesh(const int SEG_COUNT);
	void createConstantBuffers();

public:
	int SunDiskSegments = 32;

	float SunRadius = 0.004649f;
	float SunIntensity = 10.0f;

	Vector2 SunAngle = Vector2(0.0f, 11.6f);
	Vector3 SunColor = Vector3(1.0f);
	Vector3 SunDirection;
	Vector3 SunRadiance;
	Matrix SunViewProjection;

private:
	ID3D11Buffer* m_pSunDiskBuffer = nullptr;
	std::vector<Vector2> m_SunDiskVertices;
	ConstantBuffer* m_pSunVSConstantBuffer = nullptr;
	ConstantBuffer* m_pSunPSConstantBuffer = nullptr;

	// Datas.
	float m_WorldScale = 0.0f;
	Vector3 m_CameraEye;
	Matrix m_CameraViewProjection;

	// DO NOT release directly.
	BaseRenderer* m_pRenderer = nullptr;
	ConstantBuffer* m_pAtmosphereConstantBuffer = nullptr;
	Texture* m_pTransmittanceLUT = nullptr;
};
