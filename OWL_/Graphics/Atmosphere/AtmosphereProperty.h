#pragma once

ALIGN(16) class AtmosphereProperty
{
public:
	AtmosphereProperty() = default;
	~AtmosphereProperty() = default;

    AtmosphereProperty ToStdUnit();

    Vector3 GetSigmaS(const float H);

    Vector3 GetSigmaT(const float H);

    Vector3 EvalPhaseFunction(const float H, const float U);

public:
    Vector3 ScatterRayleigh = Vector3(5.802f, 13.558f, 33.1f);
    float hDensityRayleigh = 8.0f;

    float ScatterMie = 3.996f;
    float AsymmetryMie = 0.8f;
    float AbsorbMie = 4.4f;
    float hDensityMie = 1.2f;

    Vector3 AbsorbOzone = Vector3(0.65f, 1.881f, 0.085f);
    float OzoneCenterHeight = 25.0f;

    float OzoneThickness = 30.0f;
    float PlanetRadius = 6360.0f;
    float AtmosphereRadius = 6460.0f;
    float pad0 = 0.0f;
};
