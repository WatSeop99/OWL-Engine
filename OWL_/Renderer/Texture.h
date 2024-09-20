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

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const D3D11_TEXTURE2D_DESC& DESC, void* pInitData, bool bCreatingViews);
	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const D3D11_TEXTURE3D_DESC& DESC, void* pInitData, bool bCreatingViews);

	void CreateRTV(const D3D11_RENDER_TARGET_VIEW_DESC& RTV_DESC);
	void CreateSRV(const D3D11_SHADER_RESOURCE_VIEW_DESC& SRV_DESC);
	void CreateDSV(const D3D11_DEPTH_STENCIL_VIEW_DESC& DSV_DESC);
	void CreateUAV(const D3D11_UNORDERED_ACCESS_VIEW_DESC& UAV_DESC);

	void Upload();

	void Cleanup();

	inline ID3D11Texture2D** GetTexture2DPtr() { return &m_pTexture2D; }
	inline ID3D11Texture3D** GetTexture3DPtr() { return &m_pTexture3D; }

protected:
	void createTexture();
	void createStagingTexture(void* pInitData);
	void createRenderTargetView();
	void createShaderResourceView();
	void createDepthStencilView();
	void createUnorderedAccessView();

public:
	ID3D11RenderTargetView* pRTV = nullptr;
	ID3D11ShaderResourceView* pSRV = nullptr;
	ID3D11DepthStencilView* pDSV = nullptr;
	ID3D11UnorderedAccessView* pUAV = nullptr;
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
	union
	{
		D3D11_TEXTURE2D_DESC m_Texture2DDesc;
		D3D11_TEXTURE3D_DESC m_Texture3DDesc;
	};

	eTextureType m_eTextureType = TextureType_Unknown;
};

