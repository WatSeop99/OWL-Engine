#pragma once

#include "Texture2D.h"
#include "Scene.h"

namespace Core
{
	class GBuffer
	{
	public:
		GBuffer();
		~GBuffer();

		void Initialize(ID3D11Device* pDevice);

		void Update(ID3D11DeviceContext* pContext);

		void Render(Scene* pScene);

	public:
		bool bisEnabled = true;
		bool bShowDebug = false;

	private:
		int m_ScreenWidth;
		int m_ScreenHeight;

		Texture2D m_AlbedoBuffer;
		Texture2D m_NormalBuffer;
		Texture2D m_PositionBuffer;
		Texture2D m_DepthBuffer;
		Texture2D m_ExtraBuffer;
		Texture2D m_ExtraBuffer2;
	};
}
