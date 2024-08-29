#include "../Common.h"
#include "ConstantDataType.h"
#include "../Renderer/ConstantBuffer.h"
#include "GraphicsUtils.h"
#include "ImageFilter.h"

void ImageFilter::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ID3D11PixelShader* pPixelShader, int width, int height)
{
	HRESULT hr = S_OK;

	m_pPixelShader = pPixelShader;

	m_Viewport = { 0, };
	m_Viewport.TopLeftX = 0;
	m_Viewport.TopLeftY = 0;
	m_Viewport.Width = (float)width;
	m_Viewport.Height = (float)height;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	ImageFilterConstData initialData;
	m_ConstantBuffer.Initialize(pDevice, pContext, sizeof(ImageFilterConstData), &initialData);

	ImageFilterConstData* pConstData = (ImageFilterConstData*)m_ConstantBuffer.pSystemMem;
	pConstData->DX = 1.0f / (float)width;
	pConstData->DY = 1.0f / (float)height;
}

void ImageFilter::UpdateConstantBuffers(ID3D11DeviceContext* pContext)
{
	_ASSERT(pContext);
	m_ConstantBuffer.Upload();
}

void ImageFilter::Render(ID3D11DeviceContext* pContext) const
{
	_ASSERT(pContext);
	_ASSERT(m_pSRVs.size() > 0);
	_ASSERT(m_pRTVs.size() > 0);

	pContext->RSSetViewports(1, &m_Viewport);
	pContext->OMSetRenderTargets((UINT)m_pRTVs.size(), m_pRTVs.data(), nullptr);

	pContext->PSSetShader(m_pPixelShader, 0, 0);
	pContext->PSSetShaderResources(0, (UINT)m_pSRVs.size(), m_pSRVs.data());
	pContext->PSSetConstantBuffers(0, 1, &m_ConstantBuffer.pBuffer);
}

void ImageFilter::Cleanup()
{
	m_pSRVs.clear();
	m_pRTVs.clear();

	m_pPixelShader = nullptr;
}

void ImageFilter::SetShaderResources(const std::vector<ID3D11ShaderResourceView*>& RESOURCES)
{
	m_pSRVs.clear();
	m_pSRVs.resize(RESOURCES.size());
	for (UINT64 i = 0, size = RESOURCES.size(); i < size; ++i)
	{
		m_pSRVs[i] = RESOURCES[i];
	}
}

void ImageFilter::SetRenderTargets(const std::vector<ID3D11RenderTargetView*>& TARGETS)
{
	m_pRTVs.clear();
	m_pRTVs.resize(TARGETS.size());
	for (UINT64 i = 0, size = TARGETS.size(); i < size; ++i)
	{
		m_pRTVs[i] = TARGETS[i];
	}
}
