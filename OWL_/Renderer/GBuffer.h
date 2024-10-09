#pragma once

#include "Texture.h"

class GBuffer
{
public:
	GBuffer() = default;
	~GBuffer() { Cleanup(); }

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const UINT WIDTH, const UINT HEIGHT);

	void Update(); // 추후 GUI 업데이트.

	void PrepareRender();
	void AfterRender();

	void Cleanup();

public:
	bool bIsEnabled = true;
	bool bShowDebug = false;

	Texture AlbedoBuffer;
	Texture NormalBuffer;
	Texture PositionBuffer;
	Texture DepthBuffer;
	Texture EmissionBuffer;
	Texture ExtraBuffer; // float4 => { metallic, roughness, ao, height }

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;
};
