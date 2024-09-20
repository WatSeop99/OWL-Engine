#pragma once

ALIGN(16) struct MultiScatteringConstants
{
	Vector3 TerainAlbedo;
	int DirSampleCount;

	Vector3 SunIntensity;
	int RayMarchStepCount;
};

class BaseRenderer;
class CosntantBuffer;
class StructuredBuffer;
class Texture;

class MultiScatteringLUT
{
private:
	const int DIR_SAMPLE_COUNT = 64;
	const int RAY_MARCH_STEP_COUNT = 256;

public:
	MultiScatteringLUT() = default;
	~MultiScatteringLUT() { Cleanup(); };

	void Initialize(BaseRenderer* pRenderer);

	void Update(const Vector3* const pTerrainAlbedo);

	void Generate(Texture* pTransmittanceLUT);

	void Cleanup();

	inline Texture* GetMultiScatteringLUT() { return m_pMultiScatteringLUT; }

	inline void SetAtmosphere(ConstantBuffer* const pAtmos) { m_pAtmosphereConstantBuffer = pAtmos; }

protected:
	void createConstantBuffer();
	void createMultiScatteringLUTBuffer();
	void createRawDiskSamples();

public:
	Vector2 Resolution = Vector2(256.0f, 256.0f);

private:
	ConstantBuffer* m_pMultiScatteringConstantBuffer = nullptr;
	Texture* m_pMultiScatteringLUT = nullptr;
	StructuredBuffer* m_pRawSamples = nullptr;

	// DO NOT release directly.
	BaseRenderer* m_pRenderer = nullptr;
	ConstantBuffer* m_pAtmosphereConstantBuffer = nullptr;
};
