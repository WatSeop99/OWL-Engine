#pragma once

#include "Texture2D.h"

namespace Graphics
{
	class GBuffer
	{
	public:
		GBuffer(Texture2D& floatBuffer, UINT width = 1280, UINT height = 720) : FinalBuffer(floatBuffer), m_ScreenWidth(width), m_ScreenHeight(height) { }
		~GBuffer() = default;

		void Initialize(ID3D11Device* pDevice);

		void Update(ID3D11DeviceContext* pContext); // 추후 GUI 업데이트.

		void PrepareRender(ID3D11DeviceContext* pContext);
		void AfterRender(ID3D11DeviceContext* pContext);

		inline void SetScreenWidth(const UINT WIDTH) { m_ScreenWidth = WIDTH; }
		inline void SetScreenHeight(const UINT HEIGHT) { m_ScreenHeight = HEIGHT; }

	public:
		bool bIsEnabled = true;
		bool bShowDebug = false;

		Texture2D AlbedoBuffer;
		Texture2D NormalBuffer;
		Texture2D PositionBuffer;
		Texture2D DepthBuffer;
		Texture2D EmissionBuffer;
		Texture2D ExtraBuffer; // float4 => { metallic, roughness, ao, height }
		Texture2D& FinalBuffer; // float buffer임.

	private:
		UINT m_ScreenWidth;
		UINT m_ScreenHeight;
	};
}
