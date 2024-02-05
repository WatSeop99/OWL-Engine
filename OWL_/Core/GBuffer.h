#pragma once

namespace Core
{
	class GBuffer
	{
	public:
		GBuffer();
		~GBuffer();

		void Initialize(ID3D11Device* pDevice);

	private:
		ID3D11Texture2D* m_pDepthBuffer = nullptr;
		ID3D11Texture2D* m_pAlbedoBuffer = nullptr;
		ID3D11Texture2D* m_pNormalBuffer = nullptr;
		ID3D11Texture2D* m_pPositionBuffer = nullptr;

	};
}
