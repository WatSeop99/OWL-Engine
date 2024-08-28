#pragma once


// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Texture.h

class Texture2D
{
public:
	Texture2D(UINT width = 1, UINT height = 1) : Width(width), Height(height) {}
	~Texture2D() { Destroy(); }

	void Initialize(ID3D11Device* pDevice, UINT width, UINT height, DXGI_FORMAT pixelFormat = DXGI_FORMAT_UNKNOWN, bool bIsDepthStencil = false);
	void Initialize(ID3D11Device* pDevice, D3D11_TEXTURE2D_DESC& desc);

	void Upload(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<uint8_t>& DATA);

	void Download(ID3D11DeviceContext* pContext, std::vector<uint8_t>& buffer);

	void Destroy();

public:
	UINT Width;
	UINT Height;

	ID3D11Texture2D* pTexture = nullptr;
	ID3D11Texture2D* pStaging = nullptr;
	ID3D11RenderTargetView* pRTV = nullptr;
	ID3D11ShaderResourceView* pSRV = nullptr;
	ID3D11DepthStencilView* pDSV = nullptr;
	ID3D11UnorderedAccessView* pUAV = nullptr;
};
