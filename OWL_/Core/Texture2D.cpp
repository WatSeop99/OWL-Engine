#include "../Common.h"
#include "GraphicsUtils.h"
#include "Texture2D.h"

namespace Graphics
{
	void Texture2D::Initialize(ID3D11Device* pDevice, UINT width, UINT height, DXGI_FORMAT pixelFormat, bool bIsDepthStencil)
	{
		HRESULT hr = S_OK;

		Width = width;
		Height = height;

		Destroy();

		hr = Graphics::CreateTexture2D(pDevice, width, height, pixelFormat, bIsDepthStencil, &pTexture, &pRTV, &pSRV, &pDSV, &pUAV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(pTexture, "Texture2D::m_pTexture");
		SET_DEBUG_INFO_TO_OBJECT(pRTV, "Texture2D::m_pRTV");
		SET_DEBUG_INFO_TO_OBJECT(pSRV, "Texture2D::m_pSRV");
		SET_DEBUG_INFO_TO_OBJECT(pDSV, "Texture2D::m_pDSV");
		SET_DEBUG_INFO_TO_OBJECT(pUAV, "Texture2D::m_pUAV");
	}

	void Texture2D::Initialize(ID3D11Device* pDevice, D3D11_TEXTURE2D_DESC& desc)
	{
		HRESULT hr = S_OK;

		Destroy();

		Width = desc.Width;
		Height = desc.Height;

		hr = Graphics::CreateTexture2D(pDevice, desc, &pTexture, &pRTV, &pSRV, &pDSV, &pUAV);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(pTexture, "Texture2D::m_pTexture");
		SET_DEBUG_INFO_TO_OBJECT(pRTV, "Texture2D::m_pRTV");
		SET_DEBUG_INFO_TO_OBJECT(pSRV, "Texture2D::m_pSRV");
		SET_DEBUG_INFO_TO_OBJECT(pDSV, "Texture2D::m_pDSV");
		SET_DEBUG_INFO_TO_OBJECT(pUAV, "Texture2D::m_pUAV");
	}

	void Texture2D::Upload(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<uint8_t>& DATA)
	{
		_ASSERT(pTexture != nullptr);

		D3D11_TEXTURE2D_DESC desc = { 0, };
		pTexture->GetDesc(&desc);

		if (pStaging == nullptr)
		{
			pStaging = Graphics::CreateStagingTexture(pDevice, pContext, desc.Width, desc.Height, DATA, desc.Format, desc.MipLevels, desc.ArraySize);
			SET_DEBUG_INFO_TO_OBJECT(pStaging, "Texture2D::m_pStaging");
		}
		else
		{
			size_t pixelSize = Graphics::GetPixelSize(desc.Format);

			D3D11_MAPPED_SUBRESOURCE mappedResource = { 0, };
			pContext->Map(pStaging, 0, D3D11_MAP_WRITE, 0, &mappedResource);

			const uint8_t* pSRC = (uint8_t*)DATA.data();
			uint8_t* dst = (uint8_t*)mappedResource.pData;
			for (UINT j = 0; j < desc.Height; ++j)
			{
				memcpy(&dst[j * mappedResource.RowPitch], &pSRC[(j * desc.Width) * pixelSize], desc.Width * pixelSize);
			}

			pContext->Unmap(pStaging, 0);
		}

		pContext->CopyResource(pTexture, pStaging);
	}

	void Texture2D::Download(ID3D11DeviceContext* pContext, std::vector<uint8_t>& buffer)
	{
		_ASSERT(pTexture != nullptr);
		_ASSERT(pStaging != nullptr);

		pContext->CopyResource(pStaging, pTexture);

		D3D11_MAPPED_SUBRESOURCE mappedResource = { 0, };
		pContext->Map(pStaging, 0, D3D11_MAP_READ, 0, &mappedResource); // D3D11_MAP_READ ÁÖÀÇ.
		memcpy(buffer.data(), (uint8_t*)mappedResource.pData, buffer.size());
		pContext->Unmap(pStaging, 0);
	}

	void Texture2D::Destroy()
	{
		SAFE_RELEASE(pTexture);
		SAFE_RELEASE(pStaging);
		SAFE_RELEASE(pRTV);
		SAFE_RELEASE(pSRV);
		SAFE_RELEASE(pDSV);
		SAFE_RELEASE(pUAV);
	}
}