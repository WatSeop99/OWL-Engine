#pragma once

namespace Core
{
	// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Texture.h

	class Texture3D
	{
	public:
		Texture3D() : m_Width(1), m_Height(1), m_Depth(1) { }
		~Texture3D() { Destroy(); }

		void Initialize(ID3D11Device* pDevice, UINT width, UINT height, UINT depth, DXGI_FORMAT pixelFormat = DXGI_FORMAT_UNKNOWN, bool bIsDepthStencil = false)
		{
			Initialize(pDevice, width, height, depth, pixelFormat, bIsDepthStencil, {});
		}
		void Initialize(ID3D11Device* pDevice, UINT width, UINT height, UINT depth, DXGI_FORMAT pixelFormat, bool bIsDepthStencil, const std::vector<float>& INIT_DATA);
		void Initialize(ID3D11Device* pDevice, D3D11_TEXTURE3D_DESC&desc, const std::vector<float>& INIT_DATA);
		void InitNoiseF16(ID3D11Device* pDevice);

		void Upload(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<float>& data);

		void Destroy();

		inline ID3D11Texture3D* GetTexture() { return m_pTexture; }
		inline ID3D11Texture3D** GetAddressOfTexture() { return &m_pTexture; }

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
		UINT m_Depth;

		ID3D11Texture3D* m_pTexture = nullptr;
		ID3D11Texture3D* m_pStaging = nullptr;
		ID3D11RenderTargetView* m_pRTV = nullptr;
		ID3D11ShaderResourceView* m_pSRV = nullptr;
		ID3D11DepthStencilView* m_pDSV = nullptr;
		ID3D11UnorderedAccessView* m_pUAV = nullptr;
	};
}
