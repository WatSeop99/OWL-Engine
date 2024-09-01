#include "../Common.h"
#include "../Graphics/GraphicsUtils.h"
#include "Texture.h"

void Texture::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const D3D11_TEXTURE2D_DESC& DESC, void* pInitData, bool bCPUAccessable)
{
	_ASSERT(pDevice);
	_ASSERT(pContext);

	m_pDevice = pDevice;
	m_pDevice->AddRef();
	m_pContext = pContext;
	m_pContext->AddRef();

	m_Texture2DDesc = DESC;
	m_eTextureType = TextureType_Texture2D;
	m_bCPUAccessable = bCPUAccessable;

	if (bCPUAccessable)
	{
		createStagingTexture(pInitData);
	}

	// Create texture resources.
	createTexture();
	if (m_Texture2DDesc.BindFlags & D3D11_BIND_RENDER_TARGET)
	{
		createRenderTargetView();
	}
	if (m_Texture2DDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
	{
		createShaderResourceView();
	}
	if (m_Texture2DDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
	{
		createDepthStencilView();
	}
	if (m_Texture2DDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
	{
		createUnorderedAccessView();
	}
}

void Texture::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const D3D11_TEXTURE3D_DESC& DESC, void* pInitData, bool bCPUAccessable)
{
	_ASSERT(pDevice);
	_ASSERT(pContext);

	m_pDevice = pDevice;
	m_pDevice->AddRef();
	m_pContext = pContext;
	m_pContext->AddRef();

	m_Texture3DDesc = DESC;
	m_eTextureType = TextureType_Texture3D;
	m_bCPUAccessable = bCPUAccessable;

	if (bCPUAccessable)
	{
		createStagingTexture(pInitData);
	}

	// Create texture resources.
	createTexture();
	if (m_Texture3DDesc.BindFlags & D3D11_BIND_RENDER_TARGET)
	{
		createRenderTargetView();
	}
	if (m_Texture3DDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
	{
		createShaderResourceView();
	}
	if (m_Texture3DDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
	{
		createDepthStencilView();
	}
	if (m_Texture3DDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
	{
		createUnorderedAccessView();
	}
}

void Texture::Upload()
{

}

void Texture::Cleanup()
{
	SAFE_RELEASE(m_pRTV);
	SAFE_RELEASE(m_pSRV);
	SAFE_RELEASE(m_pDSV);
	SAFE_RELEASE(m_pUAV);

	switch (m_eTextureType)
	{
		case TextureType_Texture2D:
			SAFE_RELEASE(m_pTexture2D);
			SAFE_RELEASE(m_pStagingTexture2D);
			break;

		case TextureType_Texture3D:
			SAFE_RELEASE(m_pTexture3D);
			SAFE_RELEASE(m_pStagingTexture3D);
			break;

		case TextureType_Unknown:
		default:
			__debugbreak();
			break;
	}


	SAFE_RELEASE(m_pContext);
	SAFE_RELEASE(m_pDevice);
}

void Texture::createTexture()
{
	_ASSERT(m_pDevice);

	HRESULT hr = S_OK;

	if (m_eTextureType == TextureType_Texture2D)
	{
		if (m_Texture2DDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
		{
			DXGI_FORMAT originalFormat = m_Texture2DDesc.Format;
			DXGI_FORMAT textureFormat = DXGI_FORMAT_UNKNOWN;
			switch (m_Texture2DDesc.Format)
			{
				case DXGI_FORMAT_D32_FLOAT:
					textureFormat = DXGI_FORMAT_R32_TYPELESS;
					break;

				case DXGI_FORMAT_D24_UNORM_S8_UINT:
					textureFormat = DXGI_FORMAT_R24G8_TYPELESS;
					break;

				case DXGI_FORMAT_D16_UNORM:
					textureFormat = DXGI_FORMAT_R16_TYPELESS;
					break;

				default:
					break;
			}

			m_Texture2DDesc.Format = textureFormat;
			hr = m_pDevice->CreateTexture2D(&m_Texture2DDesc, nullptr, &m_pTexture2D);
			m_Texture2DDesc.Format = originalFormat;
		}
		else
		{
			hr = m_pDevice->CreateTexture2D(&m_Texture2DDesc, nullptr, &m_pTexture2D);
		}
	}
	else if (m_eTextureType == TextureType_Texture3D)
	{
		if (m_Texture3DDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
		{
			DXGI_FORMAT originalFormat = m_Texture3DDesc.Format;
			DXGI_FORMAT textureFormat = DXGI_FORMAT_UNKNOWN;
			switch (m_Texture3DDesc.Format)
			{
				case DXGI_FORMAT_D32_FLOAT:
					textureFormat = DXGI_FORMAT_R32_TYPELESS;
					break;

				case DXGI_FORMAT_D24_UNORM_S8_UINT:
					textureFormat = DXGI_FORMAT_R24G8_TYPELESS;
					break;

				case DXGI_FORMAT_D16_UNORM:
					textureFormat = DXGI_FORMAT_R16_TYPELESS;
					break;

				default:
					break;
			}

			m_Texture3DDesc.Format = textureFormat;
			hr = m_pDevice->CreateTexture3D(&m_Texture3DDesc, nullptr, &m_pTexture3D);
			m_Texture3DDesc.Format = originalFormat;
		}
		else
		{
			hr = m_pDevice->CreateTexture3D(&m_Texture3DDesc, nullptr, &m_pTexture3D);
		}
	}
	
	BREAK_IF_FAILED(hr);
}

void Texture::createStagingTexture(void* pInitData)
{
	_ASSERT(m_pDevice);
	_ASSERT(m_pContext);

	HRESULT hr = S_OK;
	DXGI_FORMAT textureFormat = DXGI_FORMAT_UNKNOWN;

	if (m_eTextureType == TextureType_Texture2D)
	{
		D3D11_TEXTURE2D_DESC stagingTextureDesc = m_Texture2DDesc;
		stagingTextureDesc.Usage = D3D11_USAGE_STAGING;
		stagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;

		textureFormat = stagingTextureDesc.Format;
		hr = m_pDevice->CreateTexture2D(&stagingTextureDesc, nullptr, &m_pStagingTexture2D);
	}
	else if (m_eTextureType == TextureType_Texture3D)
	{
		D3D11_TEXTURE3D_DESC stagingTextureDesc = m_Texture3DDesc;
		stagingTextureDesc.Usage = D3D11_USAGE_STAGING;
		stagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;

		textureFormat = stagingTextureDesc.Format;
		hr = m_pDevice->CreateTexture3D(&stagingTextureDesc, nullptr, &m_pStagingTexture3D);
	}
	BREAK_IF_FAILED(hr);


	if (!pInitData)
	{
		return;
	}

	const UINT64 PIXEL_SIZE = GetPixelSize(textureFormat);
	D3D11_MAPPED_SUBRESOURCE mappedResource = {};

	if (m_eTextureType == TextureType_Texture2D)
	{
		m_pContext->Map(m_pStagingTexture2D, 0, D3D11_MAP_WRITE, 0, &mappedResource);

		const UINT8* pSRC = (UINT8*)pInitData;
		UINT8* pDst = (UINT8*)mappedResource.pData;

		for (UINT i = 0; i < m_Texture2DDesc.Height; ++i)
		{
			memcpy(&pDst[i * mappedResource.RowPitch], &pSRC[(i * m_Texture2DDesc.Width) * PIXEL_SIZE], m_Texture2DDesc.Width * PIXEL_SIZE);
		}

		m_pContext->Unmap(m_pStagingTexture2D, 0);
		m_pContext->CopyResource(m_pTexture2D, m_pStagingTexture2D);
	}
	else if (m_eTextureType == TextureType_Texture3D)
	{
		m_pContext->Map(m_pStagingTexture3D, 0, D3D11_MAP_WRITE, 0, &mappedResource);

		const UINT8* pSRC = (UINT8*)pInitData;
		UINT8* pDst = (UINT8*)mappedResource.pData;
		for (UINT i = 0; i < m_Texture3DDesc.Depth; ++i)
		{
			for (UINT j = 0; j < m_Texture3DDesc.Height; ++j)
			{
				memcpy(&pDst[j * mappedResource.RowPitch + i * mappedResource.DepthPitch],
					   &pSRC[(j * m_Texture3DDesc.Width + i * m_Texture3DDesc.Width * m_Texture3DDesc.Height) * PIXEL_SIZE],
					   m_Texture3DDesc.Width * PIXEL_SIZE);
			}
		}

		m_pContext->Unmap(m_pStagingTexture3D, 0);
		m_pContext->CopyResource(m_pTexture3D, m_pStagingTexture3D);
	}
}

void Texture::createRenderTargetView()
{
	_ASSERT(m_pDevice);
	
	HRESULT hr = S_OK;

	if (m_eTextureType == TextureType_Texture2D)
	{
		_ASSERT(m_pTexture2D);
		hr = m_pDevice->CreateRenderTargetView(m_pTexture2D, nullptr, &m_pRTV);
	}
	else if (m_eTextureType == TextureType_Texture3D)
	{
		_ASSERT(m_pTexture3D);
		hr = m_pDevice->CreateRenderTargetView(m_pTexture3D, nullptr, &m_pRTV);
	}
	BREAK_IF_FAILED(hr);
}

void Texture::createShaderResourceView()
{
	_ASSERT(m_pDevice);

	HRESULT hr = S_OK;

	if (m_eTextureType == TextureType_Texture2D)
	{
		_ASSERT(m_pTexture2D);

		if (m_Texture2DDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
		{
			DXGI_FORMAT srvFormat = DXGI_FORMAT_UNKNOWN;
			switch (m_Texture2DDesc.Format)
			{
				case DXGI_FORMAT_D32_FLOAT:
					srvFormat = DXGI_FORMAT_R32_FLOAT;
					break;

				case DXGI_FORMAT_D24_UNORM_S8_UINT:
					srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
					break;

				case DXGI_FORMAT_D16_UNORM:
					srvFormat = DXGI_FORMAT_R16_UNORM;
					break;

				default:
					break;
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = srvFormat;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;

			hr = m_pDevice->CreateShaderResourceView(m_pTexture2D, &srvDesc, &m_pSRV);
		}
		else
		{
			hr = m_pDevice->CreateShaderResourceView(m_pTexture2D, nullptr, &m_pSRV);
		}
	}
	else if (m_eTextureType == TextureType_Texture3D)
	{
		_ASSERT(m_pTexture3D);
		if (m_Texture3DDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
		{
			DXGI_FORMAT srvFormat = DXGI_FORMAT_UNKNOWN;
			switch (m_Texture3DDesc.Format)
			{
				case DXGI_FORMAT_D32_FLOAT:
					srvFormat = DXGI_FORMAT_R32_FLOAT;
					break;

				case DXGI_FORMAT_D24_UNORM_S8_UINT:
					srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
					break;

				case DXGI_FORMAT_D16_UNORM:
					srvFormat = DXGI_FORMAT_R16_UNORM;
					break;

				default:
					break;
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = srvFormat;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MipLevels = 1;

			hr = m_pDevice->CreateShaderResourceView(m_pTexture3D, &srvDesc, &m_pSRV);
		}
		else
		{
			hr = m_pDevice->CreateShaderResourceView(m_pTexture3D, nullptr, &m_pSRV);
		}
	}
	BREAK_IF_FAILED(hr);
}

void Texture::createDepthStencilView()
{
	_ASSERT(m_pDevice);

	HRESULT hr = S_OK;

	if (m_eTextureType == TextureType_Texture2D)
	{
		_ASSERT(m_pTexture2D);

		DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
		switch (m_Texture2DDesc.Format)
		{
			case DXGI_FORMAT_D32_FLOAT:
				dsvFormat = DXGI_FORMAT_D32_FLOAT;
				break;

			case DXGI_FORMAT_D24_UNORM_S8_UINT:
				dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
				break;

			case DXGI_FORMAT_D16_UNORM:
				dsvFormat = DXGI_FORMAT_D16_UNORM;
				break;

			default:
				break;
		}

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = dsvFormat;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

		hr = m_pDevice->CreateDepthStencilView(m_pTexture2D, &dsvDesc, &m_pDSV);
	}
	else if (m_eTextureType == TextureType_Texture3D)
	{
		_ASSERT(m_pTexture3D);

		DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
		switch (m_Texture3DDesc.Format)
		{
			case DXGI_FORMAT_D32_FLOAT:
				dsvFormat = DXGI_FORMAT_D32_FLOAT;
				break;

			case DXGI_FORMAT_D24_UNORM_S8_UINT:
				dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
				break;

			case DXGI_FORMAT_D16_UNORM:
				dsvFormat = DXGI_FORMAT_D16_UNORM;
				break;

			default:
				break;
		}

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = dsvFormat;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		hr = m_pDevice->CreateDepthStencilView(m_pTexture3D, nullptr, &m_pDSV);
	}

	BREAK_IF_FAILED(hr);
}

void Texture::createUnorderedAccessView()
{
	_ASSERT(m_pDevice);

	HRESULT hr = S_OK;

	if (m_eTextureType == TextureType_Texture2D)
	{
		_ASSERT(m_pTexture2D);
		hr = m_pDevice->CreateUnorderedAccessView(m_pTexture2D, nullptr, &m_pUAV);
	}
	else if (m_eTextureType == TextureType_Texture3D)
	{
		_ASSERT(m_pTexture3D);
		hr = m_pDevice->CreateUnorderedAccessView(m_pTexture3D, nullptr, &m_pUAV);
	}
	BREAK_IF_FAILED(hr);
}
