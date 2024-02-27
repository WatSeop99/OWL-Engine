#pragma once

namespace Graphics
{
	// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Texture.h

	class Texture3D
	{
	public:
		Texture3D() : Width(1), Height(1), Depth(1) { }
		~Texture3D() { Destroy(); }

		void Initialize(ID3D11Device* pDevice, UINT width, UINT height, UINT depth, DXGI_FORMAT pixelFormat = DXGI_FORMAT_UNKNOWN, bool bIsDepthStencil = false)
		{ Initialize(pDevice, width, height, depth, pixelFormat, bIsDepthStencil, {}); }
		void Initialize(ID3D11Device* pDevice, UINT width, UINT height, UINT depth, DXGI_FORMAT pixelFormat, bool bIsDepthStencil, const std::vector<float>& INIT_DATA);
		void Initialize(ID3D11Device* pDevice, D3D11_TEXTURE3D_DESC&desc, const std::vector<float>& INIT_DATA);
		void InitNoiseF16(ID3D11Device* pDevice);

		void Upload(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<float>& data);

		void Destroy();

	public:
		UINT Width;
		UINT Height;
		UINT Depth;

		ID3D11Texture3D* pTexture = nullptr;
		ID3D11Texture3D* pStaging = nullptr;
		ID3D11RenderTargetView* pRTV = nullptr;
		ID3D11ShaderResourceView* pSRV = nullptr;
		ID3D11DepthStencilView* pDSV = nullptr;
		ID3D11UnorderedAccessView* pUAV = nullptr;
	};
}
