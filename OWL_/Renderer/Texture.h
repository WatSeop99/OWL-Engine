#pragma once

enum eTextureType
{
	TextureType_Unknown = 0,
	TextureType_Texture2D,
	TextureType_Texture3D,
};

class Texture
{
public:
	Texture() = default;
	~Texture() { Cleanup(); }

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const D3D11_TEXTURE2D_DESC& DESC, void* pInitData, bool bCPUAccessable = false);
	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const D3D11_TEXTURE3D_DESC& DESC, void* pInitData, bool bCPUAccessable = false);

	void Upload();

	void Cleanup();

protected:
	void createTexture();
	void createStagingTexture(void* pInitData);
	void createRenderTargetView();
	void createShaderResourceView();
	void createDepthStencilView();
	void createUnorderedAccessView();

public:
	void* pSystemMem = nullptr;

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;
	
	union
	{
		ID3D11Texture2D* m_pTexture2D;
		ID3D11Texture3D* m_pTexture3D;
	};
	union
	{
		ID3D11Texture2D* m_pStagingTexture2D;
		ID3D11Texture3D* m_pStagingTexture3D;
	};
	ID3D11RenderTargetView* m_pRTV = nullptr;
	ID3D11ShaderResourceView* m_pSRV = nullptr;
	ID3D11DepthStencilView* m_pDSV = nullptr;
	ID3D11UnorderedAccessView* m_pUAV = nullptr;
	union
	{
		D3D11_TEXTURE2D_DESC m_Texture2DDesc;
		D3D11_TEXTURE3D_DESC m_Texture3DDesc;
	};

	eTextureType m_eTextureType = TextureType_Unknown;
	bool m_bCPUAccessable = false;
};

