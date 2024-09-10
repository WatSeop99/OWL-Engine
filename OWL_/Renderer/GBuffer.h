#pragma once

#include "Texture.h"

class GBuffer
{
public:
	GBuffer(Texture& floatBuffer, UINT width = 1280, UINT height = 720) : FinalBuffer(floatBuffer), m_ScreenWidth(width), m_ScreenHeight(height) {}
	~GBuffer() { Cleanup(); }

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

	void Update(); // 추후 GUI 업데이트.

	void PrepareRender(ID3D11DepthStencilView* pDSV);
	void AfterRender();

	void Cleanup();

	inline void SetScreenWidth(const UINT WIDTH) { m_ScreenWidth = WIDTH; }
	inline void SetScreenHeight(const UINT HEIGHT) { m_ScreenHeight = HEIGHT; }

public:
	bool bIsEnabled = true;
	bool bShowDebug = false;

	Texture AlbedoBuffer;
	Texture NormalBuffer;
	Texture PositionBuffer;
	Texture DepthBuffer;
	Texture EmissionBuffer;
	Texture ExtraBuffer; // float4 => { metallic, roughness, ao, height }
	Texture& FinalBuffer; // float buffer임.

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;

	UINT m_ScreenWidth;
	UINT m_ScreenHeight;
};
