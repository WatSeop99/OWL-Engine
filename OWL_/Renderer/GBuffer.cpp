#include "../Common.h"
#include "../Graphics/GraphicsUtils.h"
#include "GBuffer.h"

void GBuffer::Initialize(ID3D11Device* pDevice)
{
	HRESULT hr = S_OK;

	D3D11_TEXTURE2D_DESC desc = { 0, };
	desc.Width = m_ScreenWidth;
	desc.Height = m_ScreenHeight;
	desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	hr = CreateTexture2D(pDevice, desc, &(AlbedoBuffer.pTexture), &(AlbedoBuffer.pRTV), &(AlbedoBuffer.pSRV), &(AlbedoBuffer.pDSV), &(AlbedoBuffer.pUAV));
	BREAK_IF_FAILED(hr);

	desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	hr = CreateTexture2D(pDevice, desc, &(NormalBuffer.pTexture), &(NormalBuffer.pRTV), &(NormalBuffer.pSRV), &(NormalBuffer.pDSV), &(NormalBuffer.pUAV));
	BREAK_IF_FAILED(hr);

	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	hr = CreateTexture2D(pDevice, desc, &(PositionBuffer.pTexture), &(PositionBuffer.pRTV), &(PositionBuffer.pSRV), &(PositionBuffer.pDSV), &(PositionBuffer.pUAV));
	BREAK_IF_FAILED(hr);

	desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
	hr = CreateTexture2D(pDevice, desc, &(EmissionBuffer.pTexture), &(EmissionBuffer.pRTV), &(EmissionBuffer.pSRV), &(EmissionBuffer.pDSV), &(EmissionBuffer.pUAV));
	BREAK_IF_FAILED(hr);

	desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
	hr = CreateTexture2D(pDevice, desc, &(ExtraBuffer.pTexture), &(ExtraBuffer.pRTV), &(ExtraBuffer.pSRV), &(ExtraBuffer.pDSV), &(ExtraBuffer.pUAV));
	BREAK_IF_FAILED(hr);

	/*desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	hr = CreateTexture2D(pDevice, desc, &(DepthBuffer.pTexture), &(DepthBuffer.pRTV), &(DepthBuffer.pSRV), &(DepthBuffer.pDSV), &(DepthBuffer.pUAV));
	BREAK_IF_FAILED(hr);*/

	/*desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS;
	hr = CreateTexture2D(pDevice, desc, &(FinalBuffer.pTexture), &(FinalBuffer.pRTV), &(FinalBuffer.pSRV), &(FinalBuffer.pDSV), &(FinalBuffer.pUAV));
	BREAK_IF_FAILED(hr);*/

	// depth buffers.
	desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // DSV ±âÁØ.
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	hr = pDevice->CreateTexture2D(&desc, nullptr, &(DepthBuffer.pTexture));
	BREAK_IF_FAILED(hr);

	hr = pDevice->CreateDepthStencilView(DepthBuffer.pTexture, nullptr, &(DepthBuffer.pDSV));
	BREAK_IF_FAILED(hr);
}

void GBuffer::Update(ID3D11DeviceContext* pContext)
{}

void GBuffer::PrepareRender(ID3D11DeviceContext* pContext, ID3D11DepthStencilView* pDSV)
{
	_ASSERT(pContext != nullptr);

	const float CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	ID3D11RenderTargetView* ppRTVs[5] = { AlbedoBuffer.pRTV, NormalBuffer.pRTV, PositionBuffer.pRTV, EmissionBuffer.pRTV, ExtraBuffer.pRTV };
	pContext->ClearRenderTargetView(AlbedoBuffer.pRTV, CLEAR_COLOR);
	pContext->ClearRenderTargetView(NormalBuffer.pRTV, CLEAR_COLOR);
	pContext->ClearRenderTargetView(PositionBuffer.pRTV, CLEAR_COLOR);
	pContext->ClearRenderTargetView(EmissionBuffer.pRTV, CLEAR_COLOR);
	pContext->ClearRenderTargetView(ExtraBuffer.pRTV, CLEAR_COLOR);
	pContext->ClearDepthStencilView(DepthBuffer.pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	// pContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	pContext->OMSetRenderTargets(5, ppRTVs, DepthBuffer.pDSV);
	// pContext->OMSetRenderTargets(5, ppRTVs, pDSV);
}

void GBuffer::AfterRender(ID3D11DeviceContext* pContext)
{
	ID3D11RenderTargetView* ppRTVs[5] = { nullptr, };
	pContext->OMSetRenderTargets(5, ppRTVs, nullptr);
}

void GBuffer::Destroy()
{
	AlbedoBuffer.Destroy();
	NormalBuffer.Destroy();
	PositionBuffer.Destroy();
	DepthBuffer.Destroy();
	EmissionBuffer.Destroy();
	ExtraBuffer.Destroy();
}
