#pragma once

namespace Core
{
	// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Texture.h

	class Texture2D
	{
	public:
		Texture2D(UINT width = 1, UINT height = 1) : m_Width(width), m_Height(height) { }
		~Texture2D() { Destroy(); }
		
		void Initialize(ID3D11Device* pDevice, UINT width, UINT height, DXGI_FORMAT pixelFormat = DXGI_FORMAT_UNKNOWN, bool bIsDepthStencil = false);
		void Initialize(ID3D11Device* pDevice, D3D11_TEXTURE2D_DESC& desc);

		void Upload(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<uint8_t>& DATA);

		void Download(ID3D11DeviceContext* pContext, std::vector<uint8_t>& buffer);

		void Destroy();

		inline ID3D11Texture2D* GetTexture() { return m_pTexture; }
		inline ID3D11Texture2D** GetAddressOfTexture() { return &m_pTexture; }

		inline ID3D11RenderTargetView* GetRTV() { return m_pRTV; }
		inline ID3D11RenderTargetView** GetAddressOfRTV() { return &m_pRTV; }

		inline ID3D11ShaderResourceView* GetSRV() { return m_pSRV; }
		inline ID3D11ShaderResourceView** GetAddressOfSRV() { return &m_pSRV; }
		
		inline ID3D11DepthStencilView* GetDSV() { return m_pDSV; }
		inline ID3D11DepthStencilView** GetAddressOfDSV() { return &m_pDSV; }

		inline ID3D11UnorderedAccessView* GetUAV() { return m_pUAV; }
		inline ID3D11UnorderedAccessView** GetAddressOfUAV() { return &m_pUAV; }

	private:
		UINT m_Width;
		UINT m_Height;

		ID3D11Texture2D* m_pTexture = nullptr;
		ID3D11Texture2D* m_pStaging = nullptr;
		ID3D11RenderTargetView* m_pRTV = nullptr;
		ID3D11ShaderResourceView* m_pSRV = nullptr;
		ID3D11DepthStencilView* m_pDSV = nullptr;
		ID3D11UnorderedAccessView* m_pUAV = nullptr;
	};
}
