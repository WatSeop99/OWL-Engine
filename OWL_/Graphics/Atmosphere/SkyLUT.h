#pragma once

ALIGN(16) struct AtmospherePropertyConstants
{
	Vector3 ScatterRayleight = Vector3(5.802f, 13.558f, 33.1f);
	float DensityRayleight = 8.0f;

	float ScatterMie = 3.996f;
	float AsymmetryMie = 0.8f;
	float AbsorbMie = 4.4f;
	float DensityMie = 1.2f;

	Vector3 AbsorbOZone = Vector3(0.65f, 1.881f, 0.085f);
	float OzoneCenterHeight = 25.0f;

	float OzoneThickness = 30.0f;
	float PlanetRadius = 6360.0f;
	float AtmosphereRadius = 6460.0f;
	float pad = 0.0f;
};
ALIGN(16) struct SkyLUTConstants
{
	Vector3 AtmosEyePos;
	int LowResMarchStepCount;

	Vector3 SunDirection;
	BOOL EnableMultiScattering;

	Vector3 SunIntensity;
	float pad;
};

class ConstantBuffer;
class BaseRenderer;
class Texture;

class SkyLUT
{
public:
	SkyLUT() = default;
	~SkyLUT() { Cleanup(); };

	void Initialize(BaseRenderer* pRenderer);

	void Update();

	void Generate();

	void Resize();

	void Cleanup();

	inline Texture* GetSkyLUT() { return m_pSkyLUT; }

	void SetCamera(const Vector3* const pAtmosEyePos);
	void SetSun(const Vector3* const pDirection, const Vector3* const pIntensity);
	inline void SetAtmosphere(ConstantBuffer* const pAtmos) { m_pAtmosphereConstantBuffer = pAtmos; }
	inline void SetTransmittanceLUT(Texture* pTransmitLUT) { m_pTransmittanceLUT = pTransmitLUT; }
	inline void SetMultiScatteringLUT(Texture* pMultiScatteringLUT) { m_pMultiScatterLUT = pMultiScatteringLUT; }

protected:
	void createSkyLUTBuffer();
	void createConstantBuffers();

	void setViewport();

public:
	SkyLUTConstants* pSkyData = nullptr;
	Vector2 Resolution = Vector2(64.0f, 64.0f);

private:
	Texture* m_pSkyLUT = nullptr;
	ConstantBuffer* m_pSkyLUTConstantBuffer = nullptr;

	// DO NOT release directly.
	BaseRenderer* m_pRenderer = nullptr;
	ConstantBuffer* m_pAtmosphereConstantBuffer = nullptr;
	Texture* m_pTransmittanceLUT = nullptr;
	Texture* m_pMultiScatterLUT = nullptr;
};
