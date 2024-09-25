#include <directxtk/DDSTextureLoader.h>
#include "../Common.h"
#include "../Graphics/GraphicsUtils.h"
#include "ResourceManager.h"

void ResourceManager::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	_ASSERT(pDevice);
	_ASSERT(pContext);

	m_pDevice = pDevice;
	m_pDevice->AddRef();
	m_pContext = pContext;
	m_pContext->AddRef();

	initSamplers();
	initRasterizerStates();
	initBlendStates();
	initDepthStencilStates();
	initShaders();
	initPipelineStates();
}

HRESULT ResourceManager::CreateVertexBuffer(UINT sizePerVertex, UINT numVertex, ID3D11Buffer** ppOutVertexBuffer, void* pInitData)
{
	_ASSERT(m_pDevice);
	_ASSERT(ppOutVertexBuffer && !(*ppOutVertexBuffer));
	_ASSERT(pInitData);
	_ASSERT(sizePerVertex > 0);
	_ASSERT(numVertex > 0);

	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = sizePerVertex * numVertex;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0; // 0은 CPU 접근이 필요없음을 나타냄.
	bufferDesc.StructureByteStride = sizePerVertex;

	D3D11_SUBRESOURCE_DATA vertexBufferData = {};
	vertexBufferData.pSysMem = pInitData;

	hr = m_pDevice->CreateBuffer(&bufferDesc, &vertexBufferData, ppOutVertexBuffer);
	return hr;
}

HRESULT ResourceManager::CreateIndexBuffer(UINT sizePerIndex, UINT numIndex, ID3D11Buffer** ppOutIndexBuffer, void* pInitData)
{
	_ASSERT(m_pDevice);
	_ASSERT(ppOutIndexBuffer && !(*ppOutIndexBuffer));
	_ASSERT(pInitData);
	_ASSERT(sizePerIndex > 0);
	_ASSERT(numIndex > 0);

	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = sizePerIndex * numIndex;
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // 초기화 후 변경 x.
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.StructureByteStride = sizePerIndex;

	D3D11_SUBRESOURCE_DATA indexBufferData = {};
	indexBufferData.pSysMem = pInitData;

	hr = m_pDevice->CreateBuffer(&bufferDesc, &indexBufferData, ppOutIndexBuffer);
	return hr;
}

HRESULT ResourceManager::CreateTextureFromFile(const WCHAR* pszFileName, ID3D11Texture2D** ppOutTexture, D3D11_TEXTURE2D_DESC* pOutDesc, bool bUseSRGB)
{
	_ASSERT(m_pDevice);
	_ASSERT(pszFileName);
	_ASSERT(ppOutTexture && !(*ppOutTexture));
	_ASSERT(pOutDesc);

	HRESULT hr = S_OK;

	int width = 0;
	int height = 0;
	std::vector<UINT8> imageData;
	//DXGI_FORMAT pixelFormat = (bUseSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);
	DXGI_FORMAT pixelFormat;

	if (GetFileExtension(pszFileName).compare(L"exr") == 0)
	{
		hr = ReadEXRImage(pszFileName, imageData, &width, &height, &pixelFormat);
	}
	else
	{
		hr = ReadImage(pszFileName, imageData, &width, &height, &pixelFormat);
	}

	if (FAILED(hr))
	{
		hr = E_FAIL;
		goto LB_RET;
	}

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 0;
	textureDesc.ArraySize = 1;
	textureDesc.Format = pixelFormat;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT; // 스테이징 텍스쳐로부터 복사 가능.
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS; // 밉맵 사용.

	D3D11_SUBRESOURCE_DATA imageDataForTexture = {};
	imageDataForTexture.pSysMem = imageData.data();

	hr = m_pDevice->CreateTexture2D(&textureDesc, &imageDataForTexture, ppOutTexture);
	*pOutDesc = textureDesc;

LB_RET:
	return hr;
}

HRESULT ResourceManager::CreateTextureCubeFromFile(const WCHAR* pszFileName, ID3D11Texture2D** ppOutTexture, D3D11_TEXTURE2D_DESC* pOutDesc)
{
	_ASSERT(m_pDevice);
	_ASSERT(pszFileName);
	_ASSERT(ppOutTexture && !(*ppOutTexture));
	_ASSERT(pOutDesc);
	
	// Assume that the file is in the form of a cube map.

	HRESULT hr = S_OK;
	UINT miscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	hr = DirectX::CreateDDSTextureFromFileEx(m_pDevice, pszFileName, 0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, miscFlags, DirectX::DDS_LOADER_FLAGS(false), (ID3D11Resource**)ppOutTexture, nullptr, nullptr);
	if (SUCCEEDED(hr))
	{
		(*ppOutTexture)->GetDesc(pOutDesc);
	}

	return hr;
}

HRESULT ResourceManager::CreateTexture(const D3D11_TEXTURE2D_DESC& TEXTURE_DESC, void* pInitData, ID3D11Texture2D** ppOutTexture)
{
	_ASSERT(m_pDevice);
	_ASSERT(ppOutTexture && !(*ppOutTexture));

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pInitData;

	HRESULT hr = m_pDevice->CreateTexture2D(&TEXTURE_DESC, &initData, ppOutTexture);
	return hr;
}

void ResourceManager::SetPipelineState(const eGraphicsPSOType PSO_TYPE)
{
	_ASSERT(PSO_TYPE >= 0 && PSO_TYPE < GraphicsPSOType_Count);

	m_pContext->VSSetShader(GraphicsPSOs[PSO_TYPE].pVertexShader, nullptr, 0);
	m_pContext->PSSetShader(GraphicsPSOs[PSO_TYPE].pPixelShader, nullptr, 0);
	m_pContext->HSSetShader(GraphicsPSOs[PSO_TYPE].pHullShader, nullptr, 0);
	m_pContext->DSSetShader(GraphicsPSOs[PSO_TYPE].pDomainShader, nullptr, 0);
	m_pContext->GSSetShader(GraphicsPSOs[PSO_TYPE].pGeometryShader, nullptr, 0);
	m_pContext->CSSetShader(nullptr, nullptr, 0);
	m_pContext->IASetInputLayout(GraphicsPSOs[PSO_TYPE].pInputLayout);
	m_pContext->RSSetState(GraphicsPSOs[PSO_TYPE].pRasterizerState);
	m_pContext->OMSetBlendState(GraphicsPSOs[PSO_TYPE].pBlendState, GraphicsPSOs[PSO_TYPE].BlendFactor, 0xffffffff);
	m_pContext->OMSetDepthStencilState(GraphicsPSOs[PSO_TYPE].pDepthStencilState, GraphicsPSOs[PSO_TYPE].StencilRef);
	m_pContext->IASetPrimitiveTopology(GraphicsPSOs[PSO_TYPE].PrimitiveTopology);
}

void ResourceManager::SetPipelineState(const eComputePSOType PSO_TYPE)
{
	_ASSERT(PSO_TYPE >= 0 && PSO_TYPE < ComputePSOType_Count);

	m_pContext->CSSetShader(ComputePSOs[PSO_TYPE].pComputeShader, nullptr, 0);
}

void ResourceManager::Cleanup()
{
	// Sampler States
	SamplerStates.clear();
	SAFE_RELEASE(pLinearWrapSS);
	SAFE_RELEASE(pLinearClampSS);
	SAFE_RELEASE(pPointClampSS);
	SAFE_RELEASE(pShadowPointSS);
	SAFE_RELEASE(pShadowLinearSS);
	SAFE_RELEASE(pShadowCompareSS);
	SAFE_RELEASE(pPointWrapSS);
	SAFE_RELEASE(pLinearMirrorSS);
	SAFE_RELEASE(pSkyLUTSS);

	// Rasterizer States
	SAFE_RELEASE(pSolidRS);
	SAFE_RELEASE(pSolidCcwRS);
	SAFE_RELEASE(pWireRS);
	SAFE_RELEASE(pWireCcwRS);
	SAFE_RELEASE(pPostProcessingRS);
	SAFE_RELEASE(pSolidBothRS);
	SAFE_RELEASE(pWireBothRS);
	SAFE_RELEASE(pSolidBothCcwRS);
	SAFE_RELEASE(pWireBothCcwRS);

	// Depth Stencil States
	SAFE_RELEASE(pDrawDSS);
	SAFE_RELEASE(pMaskDSS);
	SAFE_RELEASE(pDrawMaskedDSS);
	SAFE_RELEASE(pSkyDSS);
	SAFE_RELEASE(pSunDSS);

	// Blend States
	SAFE_RELEASE(pMirrorBS);
	SAFE_RELEASE(pAccumulateBS);
	SAFE_RELEASE(pAlphaBS);

	// Shaders
	SAFE_RELEASE(pBasicVS);
	SAFE_RELEASE(pSkinnedVS);
	SAFE_RELEASE(pSkyboxVS);
	SAFE_RELEASE(pSamplingVS);
	SAFE_RELEASE(pNormalVS);
	SAFE_RELEASE(pDepthOnlyVS);
	SAFE_RELEASE(pDepthOnlySkinnedVS);
	SAFE_RELEASE(pGrassVS);
	SAFE_RELEASE(pBillboardVS);
	SAFE_RELEASE(pGBufferVS);
	SAFE_RELEASE(pGBufferSkinnedVS);
	SAFE_RELEASE(pSkyLUTVS);
	SAFE_RELEASE(pSkyVS);
	SAFE_RELEASE(pSunVS);

	SAFE_RELEASE(pBasicPS);
	SAFE_RELEASE(pSkyboxPS);
	SAFE_RELEASE(pCombinePS);
	SAFE_RELEASE(pBloomDownPS);
	SAFE_RELEASE(pBloomUpPS);
	SAFE_RELEASE(pNormalPS);
	SAFE_RELEASE(pDepthOnlyPS);
	SAFE_RELEASE(pPostEffectsPS);
	SAFE_RELEASE(pVolumeSmokePS);
	SAFE_RELEASE(pColorPS);
	SAFE_RELEASE(pGrassPS);
	SAFE_RELEASE(pOceanPS);
	SAFE_RELEASE(pVolumetricFirePS);
	SAFE_RELEASE(pExplosionPS);
	SAFE_RELEASE(pGBufferPS);
	SAFE_RELEASE(pDeferredLightingPS);
	SAFE_RELEASE(pSkyLUTPS);
	SAFE_RELEASE(pSkyPS);
	SAFE_RELEASE(pSunPS);

	SAFE_RELEASE(pNormalGS);
	SAFE_RELEASE(pBillboardGS);

	SAFE_RELEASE(pAerialLUTCS);
	SAFE_RELEASE(pMultiScatterLUTCS);
	SAFE_RELEASE(pTransmittanceLUTCS);

	// Input Layouts
	SAFE_RELEASE(pBasicIL);
	SAFE_RELEASE(pSkinnedIL);
	SAFE_RELEASE(pSamplingIL);
	SAFE_RELEASE(pSkyboxIL);
	SAFE_RELEASE(pPostProcessingIL);
	SAFE_RELEASE(pGrassIL);
	SAFE_RELEASE(pBillboardIL);
	SAFE_RELEASE(pSunIL);

	SAFE_RELEASE(pDepthOnlyCubeVS);
	SAFE_RELEASE(pDepthOnlyCubeSkinnedVS);
	SAFE_RELEASE(pDepthOnlyCascadeVS);
	SAFE_RELEASE(pDepthOnlyCascadeSkinnedVS);
	SAFE_RELEASE(pDepthOnlyCubeGS);
	SAFE_RELEASE(pDepthOnlyCascadeGS);
	SAFE_RELEASE(pDepthOnlyCubePS);
	SAFE_RELEASE(pDepthOnlyCascadePS);


	SAFE_RELEASE(m_pContext);
	SAFE_RELEASE(m_pDevice);
}

void ResourceManager::initSamplers()
{
	_ASSERT(m_pDevice);

	HRESULT hr = S_OK;

	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0.0f;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = m_pDevice->CreateSamplerState(&sampDesc, &pLinearWrapSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pLinearWrapSS, "g_pLinearWrapSS");

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hr = m_pDevice->CreateSamplerState(&sampDesc, &pPointWrapSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pPointWrapSS, "pPointWrapSS");

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	hr = m_pDevice->CreateSamplerState(&sampDesc, &pLinearClampSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pLinearClampSS, "pLinearClampSS");

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hr = m_pDevice->CreateSamplerState(&sampDesc, &pPointClampSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pPointClampSS, "pPointClampSS");

	// shadowPointSS.
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.BorderColor[0] = 1.0f; // 큰 Z값
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hr = m_pDevice->CreateSamplerState(&sampDesc, &pShadowPointSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pShadowPointSS, "pShadowPointSS");

	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.BorderColor[0] = 1.0f; // 큰 Z값
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	hr = m_pDevice->CreateSamplerState(&sampDesc, &pShadowLinearSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pShadowLinearSS, "pShadowLinearSS");

	// shadowCompareSS, 쉐이더 안에서는 SamplerComparisonState
	// Filter = "_COMPARISON_" 주의
	// https://www.gamedev.net/forums/topic/670575-uploading-samplercomparisonstate-in-hlsl/
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.BorderColor[0] = 100.0f; // 큰 Z값
	sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = m_pDevice->CreateSamplerState(&sampDesc, &pShadowCompareSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pShadowCompareSS, "pShadowCompareSS");

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.BorderColor[4] = { 0.0f, };
	hr = m_pDevice->CreateSamplerState(&sampDesc, &pLinearMirrorSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pLinearMirrorSS, "pLinearMirrorSS");

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	hr = m_pDevice->CreateSamplerState(&sampDesc, &pSkyLUTSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pSkyLUTSS, "pSkyLUTSS");

	// 샘플러 순서가 "Common.hlsli"에서와 일관성 있어야 함
	SamplerStates.reserve(8);
	SamplerStates.push_back(pLinearWrapSS);    // s0
	SamplerStates.push_back(pLinearClampSS);   // s1
	SamplerStates.push_back(pShadowPointSS);   // s2
	SamplerStates.push_back(pShadowLinearSS);  // s3
	SamplerStates.push_back(pShadowCompareSS); // s4
	SamplerStates.push_back(pPointWrapSS);     // s5
	SamplerStates.push_back(pLinearMirrorSS);  // s6
	SamplerStates.push_back(pPointClampSS);    // s7
}

void ResourceManager::initRasterizerStates()
{
	_ASSERT(m_pDevice);

	HRESULT hr = S_OK;

	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
	rasterDesc.FrontCounterClockwise = FALSE;
	rasterDesc.DepthClipEnable = TRUE;
	rasterDesc.MultisampleEnable = TRUE; // MSAA.
	hr = m_pDevice->CreateRasterizerState(&rasterDesc, &pSolidRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pSolidRS, "pSolidRS");

	// 거울에 반사되면 삼각형의 Winding이 바뀌기 때문에 CCW로 그려야함
	rasterDesc.FrontCounterClockwise = TRUE;
	hr = m_pDevice->CreateRasterizerState(&rasterDesc, &pSolidCcwRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pSolidCcwRS, "pSolidCcwRS");

	rasterDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;
	hr = m_pDevice->CreateRasterizerState(&rasterDesc, &pWireCcwRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pWireCcwRS, "pWireCcwRS");

	rasterDesc.FrontCounterClockwise = FALSE;
	hr = m_pDevice->CreateRasterizerState(&rasterDesc, &pWireRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pWireRS, "pWireRS");

	ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE; // 양면
	rasterDesc.FrontCounterClockwise = FALSE;
	rasterDesc.DepthClipEnable = TRUE;
	rasterDesc.MultisampleEnable = TRUE; // MSAA.
	hr = m_pDevice->CreateRasterizerState(&rasterDesc, &pSolidBothRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pSolidBothRS, "pSolidBothRS");

	rasterDesc.FrontCounterClockwise = TRUE;
	hr = m_pDevice->CreateRasterizerState(&rasterDesc, &pSolidBothCcwRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pSolidBothCcwRS, "pSolidBothCcwRS");

	rasterDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME; // 양면, Wire
	hr = m_pDevice->CreateRasterizerState(&rasterDesc, &pWireBothCcwRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pWireBothCcwRS, "pWireBothCcwRS");

	rasterDesc.FrontCounterClockwise = FALSE;
	hr = m_pDevice->CreateRasterizerState(&rasterDesc, &pWireBothRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pWireBothRS, "pWireBothRS");

	ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE;
	rasterDesc.FrontCounterClockwise = FALSE;
	rasterDesc.DepthClipEnable = FALSE;
	hr = m_pDevice->CreateRasterizerState(&rasterDesc, &pPostProcessingRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pPostProcessingRS, "pPostProcessingRS");
}

void ResourceManager::initBlendStates()
{
	_ASSERT(m_pDevice);

	HRESULT hr = S_OK;

	D3D11_BLEND_DESC mirrorBlendDesc = {};
	mirrorBlendDesc.AlphaToCoverageEnable = TRUE;
	mirrorBlendDesc.IndependentBlendEnable = FALSE;
	// 개별 RenderTarget에 대해서 설정 (최대 8개).
	mirrorBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	mirrorBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_BLEND_FACTOR;
	mirrorBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_BLEND_FACTOR;
	mirrorBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	mirrorBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	mirrorBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	mirrorBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	// 필요하면 RGBA 각각에 대해서도 조절 가능.
	mirrorBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = m_pDevice->CreateBlendState(&mirrorBlendDesc, &pMirrorBS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pMirrorBS, "pMirrorBS");


	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = TRUE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_BLEND_FACTOR;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_BLEND_FACTOR; // INV 아님
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = m_pDevice->CreateBlendState(&blendDesc, &pAccumulateBS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pAccumulateBS, "pAccumulateBS");

	// Dst: 현재 백버퍼, Src: 새로 픽셀 쉐이더에서 출력.
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable = FALSE; // <- 주의: FALSE
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = m_pDevice->CreateBlendState(&blendDesc, &pAlphaBS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pAlphaBS, "pAlphaBS");
}

void ResourceManager::initDepthStencilStates()
{
	_ASSERT(m_pDevice);

	HRESULT hr = S_OK;

	// m_drawDSS: 기본 DSS
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dsDesc.StencilEnable = FALSE; // Stencil 불필요.
	dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	// 앞면에 대해서 어떻게 작동할지 설정.
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// 뒷면에 대해 어떻게 작동할지 설정 (뒷면도 그릴 경우).
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	hr = m_pDevice->CreateDepthStencilState(&dsDesc, &pDrawDSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pDrawDSS, "pDrawDSS");

	dsDesc.DepthEnable = TRUE;
	dsDesc.StencilEnable = FALSE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	hr = m_pDevice->CreateDepthStencilState(&dsDesc, &pSkyDSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pSkyDSS, "pSkyDSS");

	dsDesc.DepthEnable = TRUE;
	dsDesc.StencilEnable = FALSE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = m_pDevice->CreateDepthStencilState(&dsDesc, &pSunDSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pSunDSS, "pSunDSS");

	// Stencil에 1로 표기해주는 DSS.
	dsDesc.DepthEnable = TRUE; // 이미 그려진 물체 유지.
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dsDesc.StencilEnable = TRUE;    // Stencil 필수.
	dsDesc.StencilReadMask = 0xFF;  // 모든 비트 다 사용.
	dsDesc.StencilWriteMask = 0xFF; // 모든 비트 다 사용.
	// 앞면에 대해서 어떻게 작동할지 설정.
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	hr = m_pDevice->CreateDepthStencilState(&dsDesc, &pMaskDSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pMaskDSS, "g_pMaskDSS");

	// Stencil에 1로 표기된 경우에"만" 그리는 DSS.
	// DepthBuffer는 초기화된 상태로 가정.
	// D3D11_COMPARISON_EQUAL 이미 1로 표기된 경우에만 그리기.
	// OMSetDepthStencilState(..., 1); <- 여기의 1.
	dsDesc.DepthEnable = TRUE;   // 거울 속을 다시 그릴때 필요.
	dsDesc.StencilEnable = TRUE; // Stencil 사용.
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // <- 주의
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	hr = m_pDevice->CreateDepthStencilState(&dsDesc, &pDrawMaskedDSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pDrawMaskedDSS, "pDrawMaskedDSS");
}

void ResourceManager::initShaders()
{
	_ASSERT(m_pDevice);

	HRESULT hr = S_OK;

	// Shaders, InputLayouts
	const D3D11_INPUT_ELEMENT_DESC BASIC_IEs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	const D3D11_INPUT_ELEMENT_DESC SKINNED_IEs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 76, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 1, DXGI_FORMAT_R8G8B8A8_UINT, 0, 80, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	const D3D11_INPUT_ELEMENT_DESC SAMPLING_IEs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 + 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	const D3D11_INPUT_ELEMENT_DESC SKYBOX_IEs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	const D3D11_INPUT_ELEMENT_DESC GRASS_IEs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },

		// 행렬 하나는 4x4라서 Element 4개 사용 (쉐이더에서는 행렬 하나).
		{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // 마지막 1은 instance step
		{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // 마지막 1은 instance step
		{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // 마지막 1은 instance step
		{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48,  D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // 마지막 1은 instance step
		{ "COLOR", 0, DXGI_FORMAT_R32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
	};
	const D3D11_INPUT_ELEMENT_DESC BILLBOARD_IEs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	const D3D11_INPUT_ELEMENT_DESC SUN_IEs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	const D3D_SHADER_MACRO pSKINNED_MACRO[] =
	{
		{ "SKINNED", "1" }, { NULL, NULL }
	};
	UINT numBasicIEs = _countof(BASIC_IEs);
	UINT numSkinnedIEs = _countof(SKINNED_IEs);
	UINT numSamplingIEs = _countof(SAMPLING_IEs);
	UINT numSkyboxIEs = _countof(SKYBOX_IEs);
	UINT numGrassIEs = _countof(GRASS_IEs);
	UINT numBillboardIEs = _countof(BILLBOARD_IEs);
	UINT numSunIEs = _countof(SUN_IEs);

	hr = createVertexShaderAndInputLayout(L"./Shaders/BasicVS.hlsl", BASIC_IEs, numBasicIEs, nullptr, &pBasicVS, &pBasicIL);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/BasicVS.hlsl", SKINNED_IEs, numSkinnedIEs, pSKINNED_MACRO, &pSkinnedVS, &pSkinnedIL);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/NormalVS.hlsl", BASIC_IEs, numBasicIEs, nullptr, &pNormalVS, &pBasicIL);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/SamplingVS.hlsl", SAMPLING_IEs, numSamplingIEs, nullptr, &pSamplingVS, &pSamplingIL);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/SkyboxVS.hlsl", SKYBOX_IEs, numSkyboxIEs, nullptr, &pSkyboxVS, &pSkyboxIL);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/DepthOnlyVS.hlsl", BASIC_IEs, numBasicIEs, nullptr, &pDepthOnlyVS, &pSkyboxIL);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/DepthOnlyVS.hlsl", SKINNED_IEs, numSkinnedIEs, pSKINNED_MACRO, &pDepthOnlySkinnedVS, &pSkinnedIL);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/GrassVS.hlsl", GRASS_IEs, numGrassIEs, nullptr, &pGrassVS, &pGrassIL);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/BillboardVS.hlsl", BILLBOARD_IEs, numBillboardIEs, nullptr, &pBillboardVS, &pBillboardIL);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/GBufferVS.hlsl", BASIC_IEs, numBasicIEs, nullptr, &pGBufferVS, &pBasicIL);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/GBufferVS.hlsl", SKINNED_IEs, numSkinnedIEs, pSKINNED_MACRO, &pGBufferSkinnedVS, &pSkinnedIL);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/Atmosphere/SkyLUTVS.hlsl", nullptr, 0, nullptr, &pSkyLUTVS, nullptr);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/Atmosphere/SkyVS.hlsl", nullptr, 0, nullptr, &pSkyVS, nullptr);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/Atmosphere/SunVS.hlsl", SUN_IEs, numSunIEs, nullptr, &pSunVS, &pSunIL);
	BREAK_IF_FAILED(hr);

	hr = createPixelShader(L"./Shaders/BasicPS.hlsl", &pBasicPS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/NormalPS.hlsl", &pNormalPS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/SkyboxPS.hlsl", &pSkyboxPS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/CombinePS.hlsl", &pCombinePS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/BloomDownPS.hlsl", &pBloomDownPS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/BloomUpPS.hlsl", &pBloomUpPS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/DepthOnlyPS.hlsl", &pDepthOnlyPS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/PostEffectPS.hlsl", &pPostEffectsPS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/ColorPS.hlsl", &pColorPS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/GrassPS.hlsl", &pGrassPS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/OceanPS.hlsl", &pOceanPS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/ExplosionPS.hlsl", &pExplosionPS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/VolumetricFirePS.hlsl", &pVolumetricFirePS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/GBufferPS.hlsl", &pGBufferPS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/DeferredLightingPS.hlsl", &pDeferredLightingPS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/Atmosphere/SkyLUTPS.hlsl", &pSkyLUTPS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/Atmosphere/SkyPS.hlsl", &pSkyPS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/Atmosphere/SunPS.hlsl", &pSunPS);
	BREAK_IF_FAILED(hr);

	hr = createGeometryShader(L"./Shaders/NormalGS.hlsl", &pNormalGS);
	BREAK_IF_FAILED(hr);
	hr = createGeometryShader(L"./Shaders/BillboardGS.hlsl", &pBillboardGS);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/DepthOnlyCubeVS.hlsl", BASIC_IEs, numBasicIEs, nullptr, &pDepthOnlyCubeVS, &pSkyboxIL);
	BREAK_IF_FAILED(hr);
	hr = createVertexShaderAndInputLayout(L"./Shaders/DepthOnlyCubeVS.hlsl", SKINNED_IEs, numSkinnedIEs, pSKINNED_MACRO, &pDepthOnlyCubeSkinnedVS, &pSkinnedIL);
	BREAK_IF_FAILED(hr);
	hr = createVertexShaderAndInputLayout(L"./Shaders/DepthOnlyCascadeVS.hlsl", BASIC_IEs, numBasicIEs, nullptr, &pDepthOnlyCascadeVS, &pSkyboxIL);
	BREAK_IF_FAILED(hr);
	hr = createVertexShaderAndInputLayout(L"./Shaders/DepthONlyCascadeVS.hlsl", SKINNED_IEs, numSkinnedIEs, pSKINNED_MACRO, &pDepthOnlyCascadeSkinnedVS, &pSkinnedIL);
	BREAK_IF_FAILED(hr);
	hr = createGeometryShader(L"./Shaders/DepthOnlyCubeGS.hlsl", &pDepthOnlyCubeGS);
	BREAK_IF_FAILED(hr);
	hr = createGeometryShader(L"./Shaders/DepthOnlyCascadeGS.hlsl", &pDepthOnlyCascadeGS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/DepthOnlyCubePS.hlsl", &pDepthOnlyCubePS);
	BREAK_IF_FAILED(hr);
	hr = createPixelShader(L"./Shaders/DepthOnlyCascadePS.hlsl", &pDepthOnlyCascadePS);
	BREAK_IF_FAILED(hr);

	hr = createComputeShader(L"./Shaders/Atmosphere/AerialLUTCS.hlsl", &pAerialLUTCS);
	BREAK_IF_FAILED(hr);
	hr = createComputeShader(L"./Shaders/Atmosphere/MultiScatterLUTCS.hlsl", &pMultiScatterLUTCS);
	BREAK_IF_FAILED(hr);
	hr = createComputeShader(L"./Shaders/Atmosphere/TransmittanceLUTCS.hlsl", &pTransmittanceLUTCS);
	BREAK_IF_FAILED(hr);
}

void ResourceManager::initPipelineStates()
{
	// g_DefaultSolidPSO
	GraphicsPSOs[GraphicsPSOType_DefaultSolid].pVertexShader = pBasicVS;
	GraphicsPSOs[GraphicsPSOType_DefaultSolid].pInputLayout = pBasicIL;
	GraphicsPSOs[GraphicsPSOType_DefaultSolid].pPixelShader = pBasicPS;
	GraphicsPSOs[GraphicsPSOType_DefaultSolid].pRasterizerState = pSolidRS;
	GraphicsPSOs[GraphicsPSOType_DefaultSolid].PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// Skinned mesh solid
	GraphicsPSOs[GraphicsPSOType_SkinnedSolid] = GraphicsPSOs[GraphicsPSOType_DefaultSolid];
	GraphicsPSOs[GraphicsPSOType_SkinnedSolid].pVertexShader = pSkinnedVS;
	GraphicsPSOs[GraphicsPSOType_SkinnedSolid].pInputLayout = pSkinnedIL;

	// g_DefaultWirePSO
	GraphicsPSOs[GraphicsPSOType_DefaultWire] = GraphicsPSOs[GraphicsPSOType_DefaultSolid];
	GraphicsPSOs[GraphicsPSOType_DefaultWire].pRasterizerState = pWireRS;

	// Skinned mesh wire
	GraphicsPSOs[GraphicsPSOType_SkinnedWire] = GraphicsPSOs[GraphicsPSOType_SkinnedSolid];
	GraphicsPSOs[GraphicsPSOType_SkinnedWire].pRasterizerState = pWireRS;

	// stencilMarkPSO;
	GraphicsPSOs[GraphicsPSOType_StencilMask] = GraphicsPSOs[GraphicsPSOType_DefaultSolid];
	GraphicsPSOs[GraphicsPSOType_StencilMask].pDepthStencilState = pMaskDSS;
	GraphicsPSOs[GraphicsPSOType_StencilMask].StencilRef = 1;
	GraphicsPSOs[GraphicsPSOType_StencilMask].pVertexShader = pDepthOnlyVS;
	GraphicsPSOs[GraphicsPSOType_StencilMask].pPixelShader = pDepthOnlyPS;

	// g_ReflectSolidPSO: 반사되면 Winding 반대
	GraphicsPSOs[GraphicsPSOType_ReflectSolid] = GraphicsPSOs[GraphicsPSOType_DefaultSolid];
	GraphicsPSOs[GraphicsPSOType_ReflectSolid].pDepthStencilState = pDrawMaskedDSS;
	GraphicsPSOs[GraphicsPSOType_ReflectSolid].pRasterizerState = pSolidCcwRS; // 반시계
	GraphicsPSOs[GraphicsPSOType_ReflectSolid].StencilRef = 1;

	GraphicsPSOs[GraphicsPSOType_ReflectSkinnedSolid] = GraphicsPSOs[GraphicsPSOType_ReflectSolid];
	GraphicsPSOs[GraphicsPSOType_ReflectSkinnedSolid].pVertexShader = pSkinnedVS;
	GraphicsPSOs[GraphicsPSOType_ReflectSkinnedSolid].pInputLayout = pSkinnedIL;

	// g_ReflectWirePSO: 반사되면 Winding 반대
	GraphicsPSOs[GraphicsPSOType_ReflectWire] = GraphicsPSOs[GraphicsPSOType_ReflectSolid];
	GraphicsPSOs[GraphicsPSOType_ReflectWire].pRasterizerState = pWireCcwRS; // 반시계
	GraphicsPSOs[GraphicsPSOType_ReflectWire].StencilRef = 1;

	GraphicsPSOs[GraphicsPSOType_ReflectSkinnedWire] = GraphicsPSOs[GraphicsPSOType_ReflectSkinnedSolid];
	GraphicsPSOs[GraphicsPSOType_ReflectSkinnedWire].pRasterizerState = pWireCcwRS; // 반시계
	GraphicsPSOs[GraphicsPSOType_ReflectSkinnedWire].StencilRef = 1;

	// g_MirrorBlendSolidPSO;
	GraphicsPSOs[GraphicsPSOType_MirrorBlendSolid] = GraphicsPSOs[GraphicsPSOType_DefaultSolid];
	GraphicsPSOs[GraphicsPSOType_MirrorBlendSolid].pBlendState = pMirrorBS;
	GraphicsPSOs[GraphicsPSOType_MirrorBlendSolid].pDepthStencilState = pDrawMaskedDSS;
	GraphicsPSOs[GraphicsPSOType_MirrorBlendSolid].StencilRef = 1;

	// g_MirrorBlendWirePSO;
	GraphicsPSOs[GraphicsPSOType_MirrorBlendWire] = GraphicsPSOs[GraphicsPSOType_DefaultWire];
	GraphicsPSOs[GraphicsPSOType_MirrorBlendWire].pBlendState = pMirrorBS;
	GraphicsPSOs[GraphicsPSOType_MirrorBlendWire].pDepthStencilState = pDrawMaskedDSS;
	GraphicsPSOs[GraphicsPSOType_MirrorBlendWire].StencilRef = 1;

	// g_SkyboxSolidPSO
	GraphicsPSOs[GraphicsPSOType_SkyboxSolid] = GraphicsPSOs[GraphicsPSOType_DefaultSolid];
	GraphicsPSOs[GraphicsPSOType_SkyboxSolid].pVertexShader = pSkyboxVS;
	GraphicsPSOs[GraphicsPSOType_SkyboxSolid].pPixelShader = pSkyboxPS;
	GraphicsPSOs[GraphicsPSOType_SkyboxSolid].pInputLayout = pSkyboxIL;

	// g_SkyboxWirePSO
	GraphicsPSOs[GraphicsPSOType_SkyboxWire] = GraphicsPSOs[GraphicsPSOType_SkyboxSolid];
	GraphicsPSOs[GraphicsPSOType_SkyboxWire].pRasterizerState = pWireRS;

	// g_ReflectSkyboxSolidPSO
	GraphicsPSOs[GraphicsPSOType_ReflectSkyboxSolid] = GraphicsPSOs[GraphicsPSOType_SkyboxSolid];
	GraphicsPSOs[GraphicsPSOType_ReflectSkyboxSolid].pDepthStencilState = pDrawMaskedDSS;
	GraphicsPSOs[GraphicsPSOType_ReflectSkyboxSolid].pRasterizerState = pSolidCcwRS; // 반시계
	GraphicsPSOs[GraphicsPSOType_ReflectSkyboxSolid].StencilRef = 1;

	// g_ReflectSkyboxWirePSO
	GraphicsPSOs[GraphicsPSOType_ReflectSkyboxWire] = GraphicsPSOs[GraphicsPSOType_ReflectSkyboxSolid];
	GraphicsPSOs[GraphicsPSOType_ReflectSkyboxWire].pRasterizerState = pWireCcwRS;
	GraphicsPSOs[GraphicsPSOType_ReflectSkyboxWire].StencilRef = 1;

	// g_NormalsPSO
	GraphicsPSOs[GraphicsPSOType_Normal] = GraphicsPSOs[GraphicsPSOType_DefaultSolid];
	GraphicsPSOs[GraphicsPSOType_Normal] .pVertexShader = pNormalVS;
	GraphicsPSOs[GraphicsPSOType_Normal] .pGeometryShader = pNormalGS;
	GraphicsPSOs[GraphicsPSOType_Normal] .pPixelShader = pNormalPS;
	GraphicsPSOs[GraphicsPSOType_Normal] .PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;

	// g_DepthOnlyPSO
	GraphicsPSOs[GraphicsPSOType_DepthOnly] = GraphicsPSOs[GraphicsPSOType_DefaultSolid];
	GraphicsPSOs[GraphicsPSOType_DepthOnly].pVertexShader = pDepthOnlyVS;
	GraphicsPSOs[GraphicsPSOType_DepthOnly].pPixelShader = pDepthOnlyPS;

	// g_DepthOnlySkinnedPSO
	GraphicsPSOs[GraphicsPSOType_DepthOnlySkinned] = GraphicsPSOs[GraphicsPSOType_DepthOnly];
	GraphicsPSOs[GraphicsPSOType_DepthOnlySkinned].pVertexShader = pDepthOnlySkinnedVS;
	GraphicsPSOs[GraphicsPSOType_DepthOnlySkinned].pInputLayout = pSkinnedIL;

	// g_DepthOnlyCubePSO
	GraphicsPSOs[GraphicsPSOType_DepthOnlyCube] = GraphicsPSOs[GraphicsPSOType_DepthOnly];
	GraphicsPSOs[GraphicsPSOType_DepthOnlyCube].pVertexShader = pDepthOnlyCubeVS;
	GraphicsPSOs[GraphicsPSOType_DepthOnlyCube].pGeometryShader = pDepthOnlyCubeGS;
	GraphicsPSOs[GraphicsPSOType_DepthOnlyCube].pPixelShader = pDepthOnlyCubePS;

	// g_DepthOnlyCubeSkinnedPSO
	GraphicsPSOs[GraphicsPSOType_DepthOnlyCubeSkinned] = GraphicsPSOs[GraphicsPSOType_DepthOnlyCube];
	GraphicsPSOs[GraphicsPSOType_DepthOnlyCubeSkinned].pVertexShader = pDepthOnlyCubeSkinnedVS;
	GraphicsPSOs[GraphicsPSOType_DepthOnlyCubeSkinned].pInputLayout = pSkinnedIL;

	// g_DepthOnlyCascadePSO
	GraphicsPSOs[GraphicsPSOType_DepthOnlyCascade] = GraphicsPSOs[GraphicsPSOType_DepthOnly];
	GraphicsPSOs[GraphicsPSOType_DepthOnlyCascade].pVertexShader = pDepthOnlyCascadeVS;
	GraphicsPSOs[GraphicsPSOType_DepthOnlyCascade].pGeometryShader = pDepthOnlyCascadeGS;
	GraphicsPSOs[GraphicsPSOType_DepthOnlyCascade].pPixelShader = pDepthOnlyCascadePS;

	// g_DepthOnlyCascadeSkinnedPSO
	GraphicsPSOs[GraphicsPSOType_DepthOnlyCascadeSkinned] = GraphicsPSOs[GraphicsPSOType_DepthOnlyCascade];
	GraphicsPSOs[GraphicsPSOType_DepthOnlyCascadeSkinned].pVertexShader = pDepthOnlyCascadeSkinnedVS;
	GraphicsPSOs[GraphicsPSOType_DepthOnlyCascadeSkinned].pInputLayout = pSkinnedIL;

	// g_PostEffectsPSO
	GraphicsPSOs[GraphicsPSOType_PostEffects].pVertexShader = pSamplingVS;
	GraphicsPSOs[GraphicsPSOType_PostEffects].pPixelShader = pPostEffectsPS;
	GraphicsPSOs[GraphicsPSOType_PostEffects].pInputLayout = pSamplingIL;
	GraphicsPSOs[GraphicsPSOType_PostEffects].pRasterizerState = pPostProcessingRS;

	// g_PostProcessingPSO
	GraphicsPSOs[GraphicsPSOType_PostProcessing].pVertexShader = pSamplingVS;
	GraphicsPSOs[GraphicsPSOType_PostProcessing].pPixelShader = pDepthOnlyPS; // dummy
	GraphicsPSOs[GraphicsPSOType_PostProcessing].pInputLayout = pSamplingIL;
	GraphicsPSOs[GraphicsPSOType_PostProcessing].pRasterizerState = pPostProcessingRS;

	// g_BoundingBoxPSO
	GraphicsPSOs[GraphicsPSOType_BoundingBox] = GraphicsPSOs[GraphicsPSOType_DefaultWire];
	GraphicsPSOs[GraphicsPSOType_BoundingBox].pPixelShader = pColorPS;
	GraphicsPSOs[GraphicsPSOType_BoundingBox].PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

	// g_GrassSolidPSO
	GraphicsPSOs[GraphicsPSOType_GrassSolid] = GraphicsPSOs[GraphicsPSOType_DefaultSolid];
	GraphicsPSOs[GraphicsPSOType_GrassSolid].pVertexShader = pGrassVS;
	GraphicsPSOs[GraphicsPSOType_GrassSolid].pPixelShader = pGrassPS;
	GraphicsPSOs[GraphicsPSOType_GrassSolid].pInputLayout = pGrassIL;
	GraphicsPSOs[GraphicsPSOType_GrassSolid].pRasterizerState = pSolidBothRS; // 양면
	GraphicsPSOs[GraphicsPSOType_GrassSolid].PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// g_GrassWirePSO
	GraphicsPSOs[GraphicsPSOType_GrassWire] = GraphicsPSOs[GraphicsPSOType_GrassSolid];
	GraphicsPSOs[GraphicsPSOType_GrassWire].pRasterizerState = pWireBothRS; // 양면

	// g_OceanPSO
	GraphicsPSOs[GraphicsPSOType_Ocean] = GraphicsPSOs[GraphicsPSOType_DefaultSolid];
	GraphicsPSOs[GraphicsPSOType_Ocean].pBlendState = pAlphaBS;
	// GraphicsPSOs[GraphicsPSOType_Ocean].pRasterizerState = g_pSolidBothRS; // 양면
	GraphicsPSOs[GraphicsPSOType_Ocean].pPixelShader = pOceanPS;

	// g_GBufferPSO
	GraphicsPSOs[GraphicsPSOType_GBuffer] = GraphicsPSOs[GraphicsPSOType_DefaultSolid];
	GraphicsPSOs[GraphicsPSOType_GBuffer].pVertexShader = pGBufferVS;
	GraphicsPSOs[GraphicsPSOType_GBuffer].pPixelShader = pGBufferPS;

	// g_GBufferWirePSO
	GraphicsPSOs[GraphicsPSOType_GBufferWire] = GraphicsPSOs[GraphicsPSOType_GBuffer];
	GraphicsPSOs[GraphicsPSOType_GBufferWire].pRasterizerState = pWireRS;

	// g_GBufferSkinnedPSO
	GraphicsPSOs[GraphicsPSOType_GBufferSkinned] = GraphicsPSOs[GraphicsPSOType_GBuffer];
	GraphicsPSOs[GraphicsPSOType_GBufferSkinned].pVertexShader = pGBufferSkinnedVS;
	GraphicsPSOs[GraphicsPSOType_GBufferSkinned].pInputLayout = pSkinnedIL;

	// g_GBufferSKinnedWirePSO
	GraphicsPSOs[GraphicsPSOType_GBufferSkinnedWire] = GraphicsPSOs[GraphicsPSOType_GBufferSkinned];
	GraphicsPSOs[GraphicsPSOType_GBufferSkinnedWire].pRasterizerState = pWireRS;

	// g_DeferredRenderingPSO
	GraphicsPSOs[GraphicsPSOType_DeferredRendering] = GraphicsPSOs[GraphicsPSOType_PostProcessing];
	GraphicsPSOs[GraphicsPSOType_DeferredRendering].pPixelShader = pDeferredLightingPS;
	GraphicsPSOs[GraphicsPSOType_DeferredRendering].pRasterizerState = pSolidRS;

	GraphicsPSOs[GraphicsPSOType_SkyLUT].pVertexShader = pSkyLUTVS;
	GraphicsPSOs[GraphicsPSOType_SkyLUT].pPixelShader = pSkyLUTPS;

	GraphicsPSOs[GraphicsPSOType_Sky].pVertexShader = pSkyVS;
	GraphicsPSOs[GraphicsPSOType_Sky].pPixelShader = pSkyPS;
	GraphicsPSOs[GraphicsPSOType_Sky].pDepthStencilState = pSkyDSS;

	GraphicsPSOs[GraphicsPSOType_Sun].pVertexShader = pSunVS;
	GraphicsPSOs[GraphicsPSOType_Sun].pPixelShader = pSunPS;
	GraphicsPSOs[GraphicsPSOType_Sun].pDepthStencilState = pSunDSS;
	GraphicsPSOs[GraphicsPSOType_Sun].pInputLayout = pSunIL;

	ComputePSOs[ComputePSOType_AerialLUT].pComputeShader = pAerialLUTCS;
	
	ComputePSOs[ComputePSOType_MultiScatterLUT].pComputeShader = pMultiScatterLUTCS;

	ComputePSOs[ComputePSOType_TransmittanceLUT].pComputeShader = pTransmittanceLUTCS;
}

HRESULT ResourceManager::createVertexShaderAndInputLayout(const WCHAR* pszFileName, const D3D11_INPUT_ELEMENT_DESC* pINPUT_ELEMENTS, const UINT ELEMENT_SIZE, const D3D_SHADER_MACRO* pSHADER_MACROS, ID3D11VertexShader** ppOutVertexShader, ID3D11InputLayout** ppOutInputLayout)
{
	_ASSERT(m_pDevice);
	_ASSERT(pszFileName);
	_ASSERT(ppOutVertexShader && !(*ppOutVertexShader));

	HRESULT hr = S_OK;

	ID3DBlob* pShaderBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	UINT compileFlags = 0;

#ifdef _DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	hr = D3DCompileFromFile(pszFileName, pSHADER_MACROS, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", compileFlags, 0, &pShaderBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA((const char*)pErrorBlob->GetBufferPointer());
			pErrorBlob->Release();
		}
		goto LB_RET;
	}

	hr = m_pDevice->CreateVertexShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, ppOutVertexShader);
	if (FAILED(hr))
	{
		pShaderBlob->Release();
		goto LB_RET;
	}

	if (ppOutInputLayout && !(*ppOutInputLayout))
	{
		_ASSERT(pINPUT_ELEMENTS);
		hr = m_pDevice->CreateInputLayout(pINPUT_ELEMENTS, ELEMENT_SIZE, pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), ppOutInputLayout);
	}
	pShaderBlob->Release();

LB_RET:
	return hr;
}

HRESULT ResourceManager::createHullShader(const WCHAR* pszFileName, ID3D11HullShader** ppOutHullShader)
{
	_ASSERT(m_pDevice);
	_ASSERT(pszFileName);
	_ASSERT(ppOutHullShader && !(*ppOutHullShader));

	HRESULT hr = S_OK;

	ID3DBlob* pShaderBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	UINT compileFlags = 0;

#ifdef _DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	hr = D3DCompileFromFile(pszFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "hs_5_0", compileFlags, 0, &pShaderBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA((const char*)pErrorBlob->GetBufferPointer());
			pErrorBlob->Release();
		}
		goto LB_RET;
	}

	hr = m_pDevice->CreateHullShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, ppOutHullShader);
	pShaderBlob->Release();

LB_RET:
	return hr;
}

HRESULT ResourceManager::createDomainShader(const WCHAR* pszFileName, ID3D11DomainShader** ppOutDomainShader)
{
	_ASSERT(m_pDevice);
	_ASSERT(pszFileName);
	_ASSERT(ppOutDomainShader && !(*ppOutDomainShader));

	HRESULT hr = S_OK;

	ID3DBlob* pShaderBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	UINT compileFlags = 0;

#ifdef _DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	hr = D3DCompileFromFile(pszFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ds_5_0", compileFlags, 0, &pShaderBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA((const char*)pErrorBlob->GetBufferPointer());
			pErrorBlob->Release();
		}
		goto LB_RET;
	}

	hr = m_pDevice->CreateDomainShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, ppOutDomainShader);
	pShaderBlob->Release();

LB_RET:
	return hr;
}

HRESULT ResourceManager::createGeometryShader(const WCHAR* pszFileName, ID3D11GeometryShader** ppOutGeometryShader)
{
	_ASSERT(m_pDevice);
	_ASSERT(pszFileName);
	_ASSERT(ppOutGeometryShader && !(*ppOutGeometryShader));

	HRESULT hr = S_OK;

	ID3DBlob* pShaderBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	UINT compileFlags = 0;

#ifdef _DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	hr = D3DCompileFromFile(pszFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "gs_5_0", compileFlags, 0, &pShaderBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA((const char*)pErrorBlob->GetBufferPointer());
			pErrorBlob->Release();
		}
		goto LB_RET;
	}

	hr = m_pDevice->CreateGeometryShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, ppOutGeometryShader);
	pShaderBlob->Release();

LB_RET:
	return hr;
}

HRESULT ResourceManager::createPixelShader(const WCHAR* pszFileName, ID3D11PixelShader** ppOutPixelShader)
{
	_ASSERT(m_pDevice);
	_ASSERT(pszFileName);
	_ASSERT(ppOutPixelShader && !(*ppOutPixelShader));

	HRESULT hr = S_OK;

	ID3DBlob* pShaderBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	UINT compileFlags = 0;

#ifdef _DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	hr = D3DCompileFromFile(pszFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", compileFlags, 0, &pShaderBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob != nullptr)
		{
			OutputDebugStringA((const char*)pErrorBlob->GetBufferPointer());
			pErrorBlob->Release();
		}
		goto LB_RET;
	}

	hr = m_pDevice->CreatePixelShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, ppOutPixelShader);
	pShaderBlob->Release();

LB_RET:
	return hr;
}

HRESULT ResourceManager::createComputeShader(const WCHAR* pszFileName, ID3D11ComputeShader** ppOutComputeShader)
{
	_ASSERT(m_pDevice);
	_ASSERT(pszFileName);
	_ASSERT(ppOutComputeShader && !(*ppOutComputeShader));

	HRESULT hr = S_OK;

	ID3DBlob* pShaderBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	UINT compileFlags = 0;

#ifdef _DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	hr = D3DCompileFromFile(pszFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "cs_5_0", compileFlags, 0, &pShaderBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA((const char*)pErrorBlob->GetBufferPointer());
			pErrorBlob->Release();
		}
		goto LB_RET;
	}

	hr = m_pDevice->CreateComputeShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, ppOutComputeShader);
	pShaderBlob->Release();

LB_RET:
	return hr;
}
