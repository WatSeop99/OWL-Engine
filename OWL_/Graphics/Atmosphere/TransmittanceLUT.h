#pragma once

class BaseRenderer;
class ConstantBuffer;
class Texture;

class TransmittanceLUT
{
public:
	TransmittanceLUT() = default;
	~TransmittanceLUT() { Cleanup(); }

	void Initialize(BaseRenderer* pRenderer);

	void Generate();

	void ResetLUT();

	void Cleanup();

	inline Texture* GetTransmittanceLUT() { return m_pTransmittanceLUT; }

	inline void SetAtmosphere(ConstantBuffer* const pAtmos) { m_pAtmosphereConstantbuffer = pAtmos; }

protected:
	void createTransmittanceLUTBuffer();

public:
	Vector2 Resolution = Vector2(256.0f, 256.0f);

private:
	Texture* m_pTransmittanceLUT = nullptr;

	// DO NOT release directly.
	BaseRenderer* m_pRenderer = nullptr;
	ConstantBuffer* m_pAtmosphereConstantbuffer = nullptr;
};
