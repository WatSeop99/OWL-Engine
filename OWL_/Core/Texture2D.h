#pragma once

namespace Core
{
	// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Texture.h

	class Texture2D
	{
	public:
		Texture2D() : m_Width(1), m_Height(1), m_Depth(1) { }
		~Texture2D() { destroy(); }

		void Initialize(ID3D11Device* pDevice, UINT width, UINT height, DXGI_FORMAT pixelFormat);

		void Upload(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<uint8_t>& DATA);

		void Download(ID3D11DeviceContext* pContext, std::vector<uint8_t>& buffer);

		inline ID3D11Texture2D* GetTexture() { return m_pTexture; }
		inline ID3D11RenderTargetView* GetRTV() { return m_pRTV; }
		inline ID3D11ShaderResourceView* GetSRV() { return m_pSRV; }
		inline ID3D11UnorderedAccessView* GetUAV() { return m_pUAV; }
		inline ID3D11Texture2D** GetAddressOfTexture() { return &m_pTexture; }
		inline ID3D11RenderTargetView** GetAddressOfRTV() { return &m_pRTV; }
		inline ID3D11ShaderResourceView** GetAddressOfSRV() { return &m_pSRV; }
		inline ID3D11UnorderedAccessView** GetAddressOfUAV() { return &m_pUAV; }

	protected:
		void destroy();

	private:
		UINT m_Width;
		UINT m_Height;
		UINT m_Depth;

		ID3D11Texture2D* m_pTexture = nullptr;
		ID3D11Texture2D* m_pStaging = nullptr;
		ID3D11RenderTargetView* m_pRTV = nullptr;
		ID3D11ShaderResourceView* m_pSRV = nullptr;
		ID3D11UnorderedAccessView* m_pUAV = nullptr;
	};
}
