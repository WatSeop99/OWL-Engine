#pragma once

ALIGN(16) struct SkyConstants
{
	Vector3 FrustumA;
	Vector3 FrustumB;
	Vector3 FrustumC;
	Vector3 FrustumD;
	float pad[4];
};

class BaseRenderer;
class ConstantBuffer;
class Texture;

class Sky
{
public:
	Sky() = default;
	~Sky() { Cleanup(); };

	void Initialize(BaseRenderer* pRenderer);

	void Update();

	void Render(Texture* pSkyView);

	void Cleanup();

	void SetCamera(const FrustumDirection* pFrustumDirs);

protected:
	void createConstantBuffer();

private:
	ConstantBuffer* m_pSkyConstantBuffer = nullptr;

	// DO NOT release directly.
	BaseRenderer* m_pRenderer = nullptr;
};
