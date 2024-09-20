#include "../../Common.h"
#include "AtmosphereProperty.h"

AtmosphereProperty AtmosphereProperty::ToStdUnit()
{
    AtmosphereProperty ret = *this;
    ret.ScatterRayleigh = 1e-6f * ret.ScatterRayleigh;
    ret.hDensityRayleigh = 1e3f * ret.hDensityRayleigh;
    ret.ScatterMie = 1e-6f * ret.ScatterMie;
    ret.AbsorbMie = 1e-6f * ret.AbsorbMie;
    ret.hDensityMie = 1e3f * ret.hDensityMie;
    ret.AbsorbOzone = 1e-6f * ret.AbsorbOzone;
    ret.OzoneCenterHeight = 1e3f * ret.OzoneCenterHeight;
    ret.OzoneThickness = 1e3f * ret.OzoneThickness;
    ret.PlanetRadius = 1e3f * ret.PlanetRadius;
    ret.AtmosphereRadius = 1e3f * ret.AtmosphereRadius;

    return ret;
}

Vector3 AtmosphereProperty::GetSigmaS(const float H)
{
    const Vector3 RAY_LEIGHT = ScatterRayleigh * exp(-H / hDensityRayleigh);
    const Vector3 MIE(ScatterMie * exp(-H / hDensityMie));
    return RAY_LEIGHT + MIE;
}

Vector3 AtmosphereProperty::GetSigmaT(const float H)
{
    const Vector3 RAY_LEIGHT = ScatterRayleigh * exp(-H / hDensityRayleigh);
    const float MIE = (ScatterMie + AbsorbMie) * exp(-H / hDensityMie);
    const float OZONE_DENSITY = Max(0.0f, 1.0f - 0.5f * abs(H - OzoneCenterHeight) / OzoneThickness);
    const Vector3 OZONE = AbsorbOzone * OZONE_DENSITY;

    return RAY_LEIGHT + Vector3(MIE) + OZONE;
}

Vector3 AtmosphereProperty::EvalPhaseFunction(const float H, const float U)
{
    const Vector3 S_RAY_LEIGHT = ScatterRayleigh * exp(-H / hDensityRayleigh);
    const Vector3 S_MIE(ScatterMie * exp(-H / hDensityMie));
    const Vector3 S = S_RAY_LEIGHT + S_MIE;

    const float G = AsymmetryMie, G2 = G * G, U2 = U * U;
    const float P_RAY_LEIGHT = 3.0f / (16.0f * DirectX::XM_PI) * (1.0f + U2);

    const float M = 1.0f + G2 - 2.0f * G * U;
    const float P_MIE = 3.0f / (8.0f * DirectX::XM_PI) * (1.0f - G2) * (1.0f + U2) / ((2.0f + G2) * M * sqrt(M));

    Vector3 result;
    result.x = (S.x > 0.0f ? (P_RAY_LEIGHT * S_RAY_LEIGHT.x + P_MIE * S_MIE.x) / S.x : 0.0f);
    result.y = (S.y > 0.0f ? (P_RAY_LEIGHT * S_RAY_LEIGHT.y + P_MIE * S_MIE.y) / S.y : 0.0f);
    result.z = (S.z > 0.0f ? (P_RAY_LEIGHT * S_RAY_LEIGHT.z + P_MIE * S_MIE.z) / S.z : 0.0f);

    return result;
}
