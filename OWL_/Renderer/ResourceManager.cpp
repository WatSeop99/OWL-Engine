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
	DXGI_FORMAT pixelFormat = (bUseSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);

	if (GetFileExtension(pszFileName).compare(L"exr") == 0)
	{
		hr = ReadEXRImage(pszFileName, imageData, &width, &height, &pixelFormat);
	}
	else
	{
		hr = ReadImage(pszFileName, imageData, &width, &height);
	}
	
	if (FAILED(hr))
	{
		hr = E_FAIL;
		goto LB_RET;
	}

	{
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
	}

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

	SAFE_RELEASE(pNormalGS);
	SAFE_RELEASE(pBillboardGS);

	// Input Layouts
	SAFE_RELEASE(pBasicIL);
	SAFE_RELEASE(pSkinnedIL);
	SAFE_RELEASE(pSamplingIL);
	SAFE_RELEASE(pSkyboxIL);
	SAFE_RELEASE(pPostProcessingIL);
	SAFE_RELEASE(pGrassIL);
	SAFE_RELEASE(pBillboardIL);

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
	sampDesc.MinLOD = 0;
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
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = m_pDevice->CreateSamplerState(&sampDesc, &pLinearMirrorSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pLinearMirrorSS, "pLinearMirrorSS");

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
	const D3D11_INPUT_ELEMENT_DESC pBILLBOARD_IEs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
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
	UINT numBillboardIEs = _countof(pBILLBOARD_IEs);

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

	hr = createVertexShaderAndInputLayout(L"./Shaders/BillboardVS.hlsl", pBILLBOARD_IEs, numBillboardIEs, nullptr, &pBillboardVS, &pBillboardIL);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/GBufferVS.hlsl", BASIC_IEs, numBasicIEs, nullptr, &pGBufferVS, &pBasicIL);
	BREAK_IF_FAILED(hr);

	hr = createVertexShaderAndInputLayout(L"./Shaders/GBufferVS.hlsl", SKINNED_IEs, numSkinnedIEs, pSKINNED_MACRO, &pGBufferSkinnedVS, &pSkinnedIL);
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
}

void ResourceManager::initPipelineStates()
{
	// g_DefaultSolidPSO;
	DefaultSolidPSO.pVertexShader = pBasicVS;
	DefaultSolidPSO.pInputLayout = pBasicIL;
	DefaultSolidPSO.pPixelShader = pBasicPS;
	DefaultSolidPSO.pRasterizerState = pSolidRS;
	DefaultSolidPSO.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// Skinned mesh solid
	SkinnedSolidPSO = DefaultSolidPSO;
	SkinnedSolidPSO.pVertexShader = pSkinnedVS;
	SkinnedSolidPSO.pInputLayout = pSkinnedIL;

	// g_DefaultWirePSO
	DefaultWirePSO = DefaultSolidPSO;
	DefaultWirePSO.pRasterizerState = pWireRS;

	// Skinned mesh wire
	SkinnedWirePSO = SkinnedSolidPSO;
	SkinnedWirePSO.pRasterizerState = pWireRS;

	// stencilMarkPSO;
	StencilMaskPSO = DefaultSolidPSO;
	StencilMaskPSO.pDepthStencilState = pMaskDSS;
	StencilMaskPSO.StencilRef = 1;
	StencilMaskPSO.pVertexShader = pDepthOnlyVS;
	StencilMaskPSO.pPixelShader = pDepthOnlyPS;

	// g_ReflectSolidPSO: 반사되면 Winding 반대
	ReflectSolidPSO = DefaultSolidPSO;
	ReflectSolidPSO.pDepthStencilState = pDrawMaskedDSS;
	ReflectSolidPSO.pRasterizerState = pSolidCcwRS; // 반시계
	ReflectSolidPSO.StencilRef = 1;

	ReflectSkinnedSolidPSO = ReflectSolidPSO;
	ReflectSkinnedSolidPSO.pVertexShader = pSkinnedVS;
	ReflectSkinnedSolidPSO.pInputLayout = pSkinnedIL;

	// g_ReflectWirePSO: 반사되면 Winding 반대
	ReflectWirePSO = ReflectSolidPSO;
	ReflectWirePSO.pRasterizerState = pWireCcwRS; // 반시계
	ReflectWirePSO.StencilRef = 1;

	ReflectSkinnedWirePSO = ReflectSkinnedSolidPSO;
	ReflectSkinnedWirePSO.pRasterizerState = pWireCcwRS; // 반시계
	ReflectSkinnedWirePSO.StencilRef = 1;

	// g_MirrorBlendSolidPSO;
	MirrorBlendSolidPSO = DefaultSolidPSO;
	MirrorBlendSolidPSO.pBlendState = pMirrorBS;
	MirrorBlendSolidPSO.pDepthStencilState = pDrawMaskedDSS;
	MirrorBlendSolidPSO.StencilRef = 1;

	// g_MirrorBlendWirePSO;
	MirrorBlendWirePSO = DefaultWirePSO;
	MirrorBlendWirePSO.pBlendState = pMirrorBS;
	MirrorBlendWirePSO.pDepthStencilState = pDrawMaskedDSS;
	MirrorBlendWirePSO.StencilRef = 1;

	// g_SkyboxSolidPSO
	SkyboxSolidPSO = DefaultSolidPSO;
	SkyboxSolidPSO.pVertexShader = pSkyboxVS;
	SkyboxSolidPSO.pPixelShader = pSkyboxPS;
	SkyboxSolidPSO.pInputLayout = pSkyboxIL;

	// g_SkyboxWirePSO
	SkyboxWirePSO = SkyboxSolidPSO;
	SkyboxWirePSO.pRasterizerState = pWireRS;

	// g_ReflectSkyboxSolidPSO
	ReflectSkyboxSolidPSO = SkyboxSolidPSO;
	ReflectSkyboxSolidPSO.pDepthStencilState = pDrawMaskedDSS;
	ReflectSkyboxSolidPSO.pRasterizerState = pSolidCcwRS; // 반시계
	ReflectSkyboxSolidPSO.StencilRef = 1;

	// g_ReflectSkyboxWirePSO
	ReflectSkyboxWirePSO = ReflectSkyboxSolidPSO;
	ReflectSkyboxWirePSO.pRasterizerState = pWireCcwRS;
	ReflectSkyboxWirePSO.StencilRef = 1;

	// g_NormalsPSO
	NormalsPSO = DefaultSolidPSO;
	NormalsPSO.pVertexShader = pNormalVS;
	NormalsPSO.pGeometryShader = pNormalGS;
	NormalsPSO.pPixelShader = pNormalPS;
	NormalsPSO.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;

	// g_DepthOnlyPSO
	DepthOnlyPSO = DefaultSolidPSO;
	DepthOnlyPSO.pVertexShader = pDepthOnlyVS;
	DepthOnlyPSO.pPixelShader = pDepthOnlyPS;

	// g_DepthOnlySkinnedPSO
	DepthOnlySkinnedPSO = DepthOnlyPSO;
	DepthOnlySkinnedPSO.pVertexShader = pDepthOnlySkinnedVS;
	DepthOnlySkinnedPSO.pInputLayout = pSkinnedIL;

	// g_DepthOnlyCubePSO
	DepthOnlyCubePSO = DepthOnlyPSO;
	DepthOnlyCubePSO.pVertexShader = pDepthOnlyCubeVS;
	DepthOnlyCubePSO.pGeometryShader = pDepthOnlyCubeGS;
	DepthOnlyCubePSO.pPixelShader = pDepthOnlyCubePS;

	// g_DepthOnlyCubeSkinnedPSO
	DepthOnlyCubeSkinnedPSO = DepthOnlyCubePSO;
	DepthOnlyCubeSkinnedPSO.pVertexShader = pDepthOnlyCubeSkinnedVS;
	DepthOnlyCubeSkinnedPSO.pInputLayout = pSkinnedIL;

	// g_DepthOnlyCascadePSO
	DepthOnlyCascadePSO = DepthOnlyPSO;
	DepthOnlyCascadePSO.pVertexShader = pDepthOnlyCascadeVS;
	DepthOnlyCascadePSO.pGeometryShader = pDepthOnlyCascadeGS;
	DepthOnlyCascadePSO.pPixelShader = pDepthOnlyCascadePS;

	// g_DepthOnlyCascadeSkinnedPSO
	DepthOnlyCascadeSkinnedPSO = DepthOnlyCascadePSO;
	DepthOnlyCascadeSkinnedPSO.pVertexShader = pDepthOnlyCascadeSkinnedVS;
	DepthOnlyCascadeSkinnedPSO.pInputLayout = pSkinnedIL;

	// g_PostEffectsPSO
	PostEffectsPSO.pVertexShader = pSamplingVS;
	PostEffectsPSO.pPixelShader = pPostEffectsPS;
	PostEffectsPSO.pInputLayout = pSamplingIL;
	PostEffectsPSO.pRasterizerState = pPostProcessingRS;

	// g_PostProcessingPSO
	PostProcessingPSO.pVertexShader = pSamplingVS;
	PostProcessingPSO.pPixelShader = pDepthOnlyPS; // dummy
	PostProcessingPSO.pInputLayout = pSamplingIL;
	PostProcessingPSO.pRasterizerState = pPostProcessingRS;

	// g_BoundingBoxPSO
	BoundingBoxPSO = DefaultWirePSO;
	BoundingBoxPSO.pPixelShader = pColorPS;
	BoundingBoxPSO.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

	// g_GrassSolidPSO
	GrassSolidPSO = DefaultSolidPSO;
	GrassSolidPSO.pVertexShader = pGrassVS;
	GrassSolidPSO.pPixelShader = pGrassPS;
	GrassSolidPSO.pInputLayout = pGrassIL;
	GrassSolidPSO.pRasterizerState = pSolidBothRS; // 양면
	GrassSolidPSO.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// g_GrassWirePSO
	GrassWirePSO = GrassSolidPSO;
	GrassWirePSO.pRasterizerState = pWireBothRS; // 양면

	// g_OceanPSO
	OceanPSO = DefaultSolidPSO;
	OceanPSO.pBlendState = pAlphaBS;
	// g_OceanPSO.pRasterizerState = g_pSolidBothRS; // 양면
	OceanPSO.pPixelShader = pOceanPS;

	// g_GBufferPSO
	GBufferPSO = DefaultSolidPSO;
	GBufferPSO.pVertexShader = pGBufferVS;
	GBufferPSO.pPixelShader = pGBufferPS;

	// g_GBufferWirePSO
	GBufferWirePSO = GBufferPSO;
	GBufferWirePSO.pRasterizerState = pWireRS;

	// g_GBufferSkinnedPSO
	GBufferSkinnedPSO = GBufferPSO;
	GBufferSkinnedPSO.pVertexShader = pGBufferSkinnedVS;
	GBufferSkinnedPSO.pInputLayout = pSkinnedIL;

	// g_GBufferSKinnedWirePSO
	GBufferSKinnedWirePSO = GBufferSkinnedPSO;
	GBufferSkinnedPSO.pRasterizerState = pWireRS;

	// g_DeferredRenderingPSO
	DeferredRenderingPSO = PostProcessingPSO;
	DeferredRenderingPSO.pPixelShader = pDeferredLightingPS;
	DeferredRenderingPSO.pRasterizerState = pSolidRS;
}

HRESULT ResourceManager::createVertexShaderAndInputLayout(const WCHAR* pszFileName, const D3D11_INPUT_ELEMENT_DESC* pINPUT_ELEMENTS, const UINT ELEMENT_SIZE, const D3D_SHADER_MACRO* pSHADER_MACROS, ID3D11VertexShader** ppOutVertexShader, ID3D11InputLayout** ppOutInputLayout)
{
	_ASSERT(m_pDevice);
	_ASSERT(pszFileName);
	_ASSERT(pINPUT_ELEMENTS);
	_ASSERT(ppOutVertexShader && !(*ppOutVertexShader));
	_ASSERT(ppOutInputLayout);

	HRESULT hr = S_OK;

	ID3DBlob* pShaderBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	UINT compileFlags = 0;

#ifdef _DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	hr = D3DCompileFromFile(pszFileName, pSHADER_MACROS, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_1", compileFlags, 0, &pShaderBlob, &pErrorBlob);
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

	if (!(*ppOutInputLayout))
	{
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

	hr = D3DCompileFromFile(pszFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "hs_5_1", compileFlags, 0, &pShaderBlob, &pErrorBlob);
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

	hr = D3DCompileFromFile(pszFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ds_5_1", compileFlags, 0, &pShaderBlob, &pErrorBlob);
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

	hr = D3DCompileFromFile(pszFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "gs_5_1", compileFlags, 0, &pShaderBlob, &pErrorBlob);
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

	hr = D3DCompileFromFile(pszFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_1", compileFlags, 0, &pShaderBlob, &pErrorBlob);
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

	hr = D3DCompileFromFile(pszFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "cs_5_1", compileFlags, 0, &pShaderBlob, &pErrorBlob);
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
