#include <fp16.h>
#include <random>
#include "../Common.h"
#include "GraphicsUtils.h"
#include "Texture3D.h"


void Texture3D::Initialize(ID3D11Device* pDevice, UINT width, UINT height, UINT depth, DXGI_FORMAT pixelFormat, bool bIsDepthStencil, const std::vector<float>& INIT_DATA)
{
	HRESULT hr = S_OK;

	Destroy();

	Width = width;
	Height = height;
	Depth = depth;

	hr = CreateTexture3D(pDevice, width, height, depth, pixelFormat, bIsDepthStencil, INIT_DATA,
						 &pTexture, &pRTV, &pSRV, &pDSV, &pUAV);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pTexture, "Texture3D::m_pTexture");
	SET_DEBUG_INFO_TO_OBJECT(pRTV, "Texture3D::m_pRTV");
	SET_DEBUG_INFO_TO_OBJECT(pSRV, "Texture3D::m_pSRV");
	SET_DEBUG_INFO_TO_OBJECT(pDSV, "Texture3D::m_pDSV");
	SET_DEBUG_INFO_TO_OBJECT(pUAV, "Texture3D::m_pUAV");
}

void Texture3D::Initialize(ID3D11Device* pDevice, D3D11_TEXTURE3D_DESC& desc, const std::vector<float>& INIT_DATA)
{
	HRESULT hr = S_OK;

	Destroy();

	Width = desc.Width;
	Height = desc.Height;
	Depth = desc.Depth;

	hr = CreateTexture3D(pDevice, desc, INIT_DATA, &pTexture, &pRTV, &pSRV, &pDSV, &pUAV);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pTexture, "Texture3D::m_pTexture");
	SET_DEBUG_INFO_TO_OBJECT(pRTV, "Texture3D::m_pRTV");
	SET_DEBUG_INFO_TO_OBJECT(pSRV, "Texture3D::m_pSRV");
	SET_DEBUG_INFO_TO_OBJECT(pDSV, "Texture3D::m_pDSV");
	SET_DEBUG_INFO_TO_OBJECT(pUAV, "Texture3D::m_pUAV");
}

void Texture3D::InitNoiseF16(ID3D11Device* pDevice)
{
	const UINT width = 64;
	const UINT height = 1024;
	const UINT depth = 64;
	const UINT initialSize = width * height * depth * 4;

	std::vector<float> f32(initialSize);
	std::mt19937 gen(0);
	std::uniform_real_distribution<float> dp(0.0f, 1.0f);
	for (UINT i = 0; i < initialSize; ++i)
	{
		f32[i] = dp(gen);
	}

	std::vector<float> f16(initialSize / 2);
	uint16_t* f16Ptr = (uint16_t*)f16.data();
	for (UINT i = 0; i < initialSize; ++i)
	{
		f16Ptr[i] = fp16_ieee_from_fp32_value(f32[i]);
	}

	Initialize(pDevice, width, height, depth, DXGI_FORMAT_R16G16B16A16_FLOAT, false, f16);
}

void Texture3D::Upload(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<float>& data)
{
	_ASSERT(pTexture != nullptr);

	D3D11_TEXTURE3D_DESC desc = { 0, };
	pTexture->GetDesc(&desc);

	if (pStaging == nullptr)
	{
		pStaging = CreateStagingTexture3D(pDevice, desc.Width, desc.Height, desc.Depth, desc.Format);
		_ASSERT(pStaging != nullptr);
		SET_DEBUG_INFO_TO_OBJECT(pStaging, "Texture3D::m_pStaging");
	}

	size_t pixelSize = GetPixelSize(desc.Format);
	D3D11_MAPPED_SUBRESOURCE mappedResource = { 0, };

	pContext->Map(pStaging, 0, D3D11_MAP_WRITE, 0, &mappedResource);
	const uint8_t* src = (uint8_t*)data.data();
	uint8_t* dst = (uint8_t*)mappedResource.pData;
	for (UINT k = 0; k < desc.Depth; ++k)
	{
		for (UINT j = 0; j < desc.Height; ++j)
		{
			memcpy(&dst[j * mappedResource.RowPitch + k * mappedResource.DepthPitch],
				   &src[(j * desc.Width + k * desc.Width * desc.Height) * pixelSize],
				   desc.Width * pixelSize);
		}
	}
	pContext->Unmap(pStaging, 0);

	pContext->CopyResource(pTexture, pStaging);
}

void Texture3D::Destroy()
{
	SAFE_RELEASE(pTexture);
	SAFE_RELEASE(pStaging);
	SAFE_RELEASE(pRTV);
	SAFE_RELEASE(pSRV);
	SAFE_RELEASE(pDSV);
	SAFE_RELEASE(pUAV);
}
