#include "../Common.h"
#include "GraphicsUtils.h"
#include "Texture2D.h"

namespace Core
{
	void Texture2D::Initialize(ID3D11Device* pDevice, UINT width, UINT height, DXGI_FORMAT pixelFormat)
	{
		m_Width = width;
		m_Height = height;

		destroy();
		Graphics::CreateUATexture(pDevice,
								  width, height, pixelFormat,
								  &m_pTexture, &m_pRTV, &m_pSRV, &m_pUAV);
		SET_DEBUG_INFO_TO_OBJECT(m_pTexture, "Texture2D::m_pTexture");
		SET_DEBUG_INFO_TO_OBJECT(m_pRTV, "Texture2D::m_pRTV");
		SET_DEBUG_INFO_TO_OBJECT(m_pSRV, "Texture2D::m_pSRV");
		SET_DEBUG_INFO_TO_OBJECT(m_pUAV, "Texture2D::m_pUAV");
	}

	void Texture2D::Upload(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<uint8_t>& DATA)
	{
		D3D11_TEXTURE2D_DESC desc = { 0, };
		m_pTexture->GetDesc(&desc);

		if (!m_pStaging)
		{
			m_pStaging = Graphics::CreateStagingTexture(pDevice, pContext, desc.Width, desc.Height, DATA, desc.Format, desc.MipLevels, desc.ArraySize);
			SET_DEBUG_INFO_TO_OBJECT(m_pStaging, "Texture2D::m_pStaging");
		}
		else
		{
			size_t pixelSize = Graphics::GetPixelSize(desc.Format);

			D3D11_MAPPED_SUBRESOURCE mappedResource = { 0, };
			pContext->Map(m_pStaging, 0, D3D11_MAP_WRITE, 0, &mappedResource);

			const uint8_t* pSRC = (uint8_t*)DATA.data();
			uint8_t* dst = (uint8_t*)mappedResource.pData;
			for (UINT j = 0; j < desc.Height; ++j)
			{
				memcpy(&dst[j * mappedResource.RowPitch], &pSRC[(j * desc.Width) * pixelSize], desc.Width * pixelSize);
			}

			pContext->Unmap(m_pStaging, 0);
		}

		pContext->CopyResource(m_pTexture, m_pStaging);
	}

	void Texture2D::Download(ID3D11DeviceContext* pContext, std::vector<uint8_t>& buffer)
	{
		pContext->CopyResource(m_pStaging, m_pTexture);

		D3D11_MAPPED_SUBRESOURCE mappedResource = { 0, };
		pContext->Map(m_pStaging, 0, D3D11_MAP_READ, 0, &mappedResource); // D3D11_MAP_READ ����.
		memcpy(buffer.data(), (uint8_t*)mappedResource.pData, buffer.size());
		pContext->Unmap(m_pStaging, 0);
	}

	void Texture2D::destroy()
	{
		SAFE_RELEASE(m_pTexture);
		SAFE_RELEASE(m_pStaging);
		SAFE_RELEASE(m_pRTV);
		SAFE_RELEASE(m_pSRV);
		SAFE_RELEASE(m_pUAV);
	}
}