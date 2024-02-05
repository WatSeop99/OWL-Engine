#pragma once

namespace Core
{
	// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Texture.h

	class Texture3D
	{
	public:
		Texture3D() : m_width(1), m_height(1), m_depth(1) { }
		~Texture3D() { destroy(); }

		void Initialize(ID3D11Device* pDevice, UINT width, UINT height, UINT depth, DXGI_FORMAT pixelFormat)
		{
			Initialize(pDevice, width, height, depth, pixelFormat, {});
		}
		void Initialize(ID3D11Device* pDevice, UINT width, UINT height, UINT depth, DXGI_FORMAT pixelFormat, const std::vector<float>& INIT_DATA);

		void InitNoiseF16(ID3D11Device* pDevice);

		void Upload(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<float>& data);

		inline ID3D11Texture3D* GetTexture() { return m_pTexture; }
		inline ID3D11RenderTargetView* GetRTV() { return m_pRTV; }
		inline ID3D11ShaderResourceView* GetSRV() { return m_pSRV; }
		inline ID3D11UnorderedAccessView* GetUAV() { return m_pUAV; }
		inline ID3D11Texture3D** GetAddressOfTexture() { return &m_pTexture; }
		inline ID3D11RenderTargetView** GetAddressOfRTV() { return &m_pRTV; }
		inline ID3D11ShaderResourceView** GetAddressOfSRV() { return &m_pSRV; }
		inline ID3D11UnorderedAccessView** GetAddressOfUAV() { return &m_pUAV; }

	protected:
		void destroy();

	private:
		UINT m_width;
		UINT m_height;
		UINT m_depth;

		ID3D11Texture3D* m_pTexture = nullptr;
		ID3D11Texture3D* m_pStaging = nullptr;
		ID3D11RenderTargetView* m_pRTV = nullptr;
		ID3D11ShaderResourceView* m_pSRV = nullptr;
		ID3D11UnorderedAccessView* m_pUAV = nullptr;
	};
}
