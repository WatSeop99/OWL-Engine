#pragma once

#include "../../Renderer/ConstantBuffer.h"

ALIGN(16) struct AerialLUTConstants
{
    Vector3 SunDirection;      
    float SunTheta;

    Vector3 FrustumA;
    float MaxDistance;
    Vector3 FrustumB;
    int PerSliceStepCount;
    Vector3 FrustumC;
    int EnableMultiScattering;
    Vector3 FrustumD;
    float EyePositionY;
    Vector3 ShadowEyePosition;
    int EnableShadow;

    Matrix ShadowViewProj;
    float WorldScale;

    float pad[3];
};

struct FrustumDirection;
class BaseRenderer;
//class CosntantBuffer;
class Texture;

class AerialLUT
{
public:
	AerialLUT() = default;
    ~AerialLUT() { Cleanup(); };

    void Initialize(BaseRenderer* pRenderer);

    void Update();

    void Generate(Texture* pShadowMap);

    void Resize();

    void Cleanup();

    inline Texture* GetAerialLUT() { return m_pAerialLUT; }

    void SetCamera(const Vector3* const pEyePos, const float ATMOS_EYE_HEIGHT, const FrustumDirection* const pFrustumDirs);
    void SetSun(const Vector3* const pDirection);
    inline void SetAtmosphere(ConstantBuffer* const pAtmos) { m_pAtmosphereConstantBuffer = pAtmos; }
    inline void SetTransmittanceLUT(Texture* const pTransmitLUT) { m_pTransmittanceLUT = pTransmitLUT; }
    inline void SetMultiScatteringLUT(Texture* const pMultiScatteringLUT) { m_pMultiScatterLUT = pMultiScatteringLUT; }

protected:
    void createAerialLUTBuffer();
    void createConstantBuffer();

public:
    AerialLUTConstants* pAerialData = nullptr;
    Vector3 Resolution = Vector3(200.0f, 150.0f, 32.0f);

private:
    ConstantBuffer* m_pAerialConstantBuffer = nullptr;
    Texture* m_pAerialLUT = nullptr;

    // DO NOT release directly.
    BaseRenderer* m_pRenderer = nullptr;
    ConstantBuffer* m_pAtmosphereConstantBuffer = nullptr;
    Texture* m_pTransmittanceLUT = nullptr;
    Texture* m_pMultiScatterLUT = nullptr;
};

