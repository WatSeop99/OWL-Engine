#include "../Common.h"
#include "GraphicsCommon.h"

// Sampler States
ID3D11SamplerState* g_pLinearWrapSS = nullptr;
ID3D11SamplerState* g_pLinearClampSS = nullptr;
ID3D11SamplerState* g_pPointClampSS = nullptr;
ID3D11SamplerState* g_pShadowPointSS = nullptr;
ID3D11SamplerState* g_pShadowLinearSS = nullptr;
ID3D11SamplerState* g_pShadowCompareSS = nullptr;
ID3D11SamplerState* g_pPointWrapSS = nullptr;
ID3D11SamplerState* g_pLinearMirrorSS = nullptr;
std::vector<ID3D11SamplerState*> g_ppSamplerStates;

// Rasterizer States
ID3D11RasterizerState* g_pSolidRS = nullptr; // front only
ID3D11RasterizerState* g_pSolidCcwRS = nullptr;
ID3D11RasterizerState* g_pWireRS = nullptr;
ID3D11RasterizerState* g_pWireCcwRS = nullptr;
ID3D11RasterizerState* g_pPostProcessingRS = nullptr;
ID3D11RasterizerState* g_pSolidBothRS = nullptr; // front and back
ID3D11RasterizerState* g_pWireBothRS = nullptr;
ID3D11RasterizerState* g_pSolidBothCcwRS = nullptr;
ID3D11RasterizerState* g_pWireBothCcwRS = nullptr;

// Depth Stencil States
ID3D11DepthStencilState* g_pDrawDSS = nullptr;       // 일반적으로 그리기
ID3D11DepthStencilState* g_pMaskDSS = nullptr;       // 스텐실버퍼에 표시
ID3D11DepthStencilState* g_pDrawMaskedDSS = nullptr; // 스텐실 표시된 곳만

// Blend States
ID3D11BlendState* g_pMirrorBS = nullptr;
ID3D11BlendState* g_pAccumulateBS = nullptr;
ID3D11BlendState* g_pAlphaBS = nullptr;

// Shaders
ID3D11VertexShader* g_pBasicVS = nullptr;
ID3D11VertexShader* g_pSkinnedVS = nullptr;
ID3D11VertexShader* g_pSkyboxVS = nullptr;
ID3D11VertexShader* g_pSamplingVS = nullptr;
ID3D11VertexShader* g_pNormalVS = nullptr;
ID3D11VertexShader* g_pDepthOnlyVS = nullptr;
ID3D11VertexShader* g_pDepthOnlySkinnedVS = nullptr;
ID3D11VertexShader* g_pGrassVS = nullptr;
ID3D11VertexShader* g_pBillboardVS = nullptr;
ID3D11VertexShader* g_pGBufferVS = nullptr;
ID3D11VertexShader* g_pGBufferSkinnedVS = nullptr;

ID3D11PixelShader* g_pBasicPS = nullptr;
ID3D11PixelShader* g_pSkyboxPS = nullptr;
ID3D11PixelShader* g_pCombinePS = nullptr;
ID3D11PixelShader* g_pBloomDownPS = nullptr;
ID3D11PixelShader* g_pBloomUpPS = nullptr;
ID3D11PixelShader* g_pNormalPS = nullptr;
ID3D11PixelShader* g_pDepthOnlyPS = nullptr;
ID3D11PixelShader* g_pPostEffectsPS = nullptr;
ID3D11PixelShader* g_pVolumeSmokePS = nullptr;
ID3D11PixelShader* g_pColorPS = nullptr;
ID3D11PixelShader* g_pGrassPS = nullptr;
ID3D11PixelShader* g_pOceanPS = nullptr;
ID3D11PixelShader* g_pVolumetricFirePS = nullptr;
ID3D11PixelShader* g_pExplosionPS = nullptr;
ID3D11PixelShader* g_pGBufferPS = nullptr;
ID3D11PixelShader* g_pDeferredLightingPS = nullptr;

ID3D11GeometryShader* g_pNormalGS = nullptr;
ID3D11GeometryShader* g_pBillboardGS = nullptr;

ID3D11VertexShader* g_pDepthOnlyCubeVS = nullptr;
ID3D11VertexShader* g_pDepthOnlyCubeSkinnedVS = nullptr;
ID3D11VertexShader* g_pDepthOnlyCascadeVS = nullptr;
ID3D11VertexShader* g_pDepthOnlyCascadeSkinnedVS = nullptr;
ID3D11GeometryShader* g_pDepthOnlyCubeGS = nullptr;
ID3D11GeometryShader* g_pDepthOnlyCascadeGS = nullptr;
ID3D11PixelShader* g_pDepthOnlyCubePS = nullptr;
ID3D11PixelShader* g_pDepthOnlyCascadePS = nullptr;

// Input Layouts
ID3D11InputLayout* g_pBasicIL = nullptr;
ID3D11InputLayout* g_pSkinnedIL = nullptr;
ID3D11InputLayout* g_pSamplingIL = nullptr;
ID3D11InputLayout* g_pSkyboxIL = nullptr;
ID3D11InputLayout* g_pPostProcessingIL = nullptr;
ID3D11InputLayout* g_pGrassIL = nullptr;     // PER_INSTANCE 사용
ID3D11InputLayout* g_pBillboardIL = nullptr; // PER_INSTANCE 사용

// Graphics Pipeline States
GraphicsPSO g_DefaultSolidPSO;
GraphicsPSO g_SkinnedSolidPSO;
GraphicsPSO g_DefaultWirePSO;
GraphicsPSO g_SkinnedWirePSO;
GraphicsPSO g_StencilMaskPSO;
GraphicsPSO g_ReflectSolidPSO;
GraphicsPSO g_ReflectSkinnedSolidPSO;
GraphicsPSO g_ReflectWirePSO;
GraphicsPSO g_ReflectSkinnedWirePSO;
GraphicsPSO g_MirrorBlendSolidPSO;
GraphicsPSO g_MirrorBlendWirePSO;
GraphicsPSO g_SkyboxSolidPSO;
GraphicsPSO g_SkyboxWirePSO;
GraphicsPSO g_ReflectSkyboxSolidPSO;
GraphicsPSO g_ReflectSkyboxWirePSO;
GraphicsPSO g_NormalsPSO;
GraphicsPSO g_DepthOnlyPSO;
GraphicsPSO g_DepthOnlySkinnedPSO;
GraphicsPSO g_DepthOnlyCubePSO;
GraphicsPSO g_DepthOnlyCubeSkinnedPSO;
GraphicsPSO g_DepthOnlyCascadePSO;
GraphicsPSO g_DepthOnlyCascadeSkinnedPSO;
GraphicsPSO g_PostEffectsPSO;
GraphicsPSO g_PostProcessingPSO;
GraphicsPSO g_BoundingBoxPSO;
GraphicsPSO g_GrassSolidPSO;
GraphicsPSO g_GrassWirePSO;
GraphicsPSO g_OceanPSO;
GraphicsPSO g_GBufferPSO;
GraphicsPSO g_GBufferWirePSO;
GraphicsPSO g_GBufferSkinnedPSO;
GraphicsPSO g_GBufferSKinnedWirePSO;
GraphicsPSO g_DeferredRenderingPSO;

// 주의: 초기화가 느려서 필요한 경우에만 초기화
GraphicsPSO g_VolumeSmokePSO;

// Compute Pipeline States

void InitCommonStates(ID3D11Device* pDevice)
{
	InitShaders(pDevice);
	InitSamplers(pDevice);
	InitRasterizerStates(pDevice);
	InitBlendStates(pDevice);
	InitDepthStencilStates(pDevice);
	InitPipelineStates(pDevice);
}

void InitSamplers(ID3D11Device* pDevice)
{
	_ASSERT(pDevice);

	HRESULT hr = S_OK;

	D3D11_SAMPLER_DESC sampDesc = {};
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = pDevice->CreateSamplerState(&sampDesc, &g_pLinearWrapSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pLinearWrapSS, "g_pLinearWrapSS");

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hr = pDevice->CreateSamplerState(&sampDesc, &g_pPointWrapSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pPointWrapSS, "g_pPointWrapSS");

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	hr = pDevice->CreateSamplerState(&sampDesc, &g_pLinearClampSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pLinearClampSS, "g_pLinearClampSS");

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hr = pDevice->CreateSamplerState(&sampDesc, &g_pPointClampSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pPointClampSS, "g_pPointClampSS");

	// shadowPointSS.
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.BorderColor[0] = 1.0f; // 큰 Z값
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hr = pDevice->CreateSamplerState(&sampDesc, &g_pShadowPointSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pShadowPointSS, "g_pShadowPointSS");

	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.BorderColor[0] = 1.0f; // 큰 Z값
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	hr = pDevice->CreateSamplerState(&sampDesc, &g_pShadowLinearSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pShadowLinearSS, "g_pShadowLinearSS");

	// shadowCompareSS, 쉐이더 안에서는 SamplerComparisonState
	// Filter = "_COMPARISON_" 주의
	// https://www.gamedev.net/forums/topic/670575-uploading-samplercomparisonstate-in-hlsl/
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.BorderColor[0] = 100.0f; // 큰 Z값
	sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = pDevice->CreateSamplerState(&sampDesc, &g_pShadowCompareSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pShadowCompareSS, "g_pShadowCompareSS");

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = pDevice->CreateSamplerState(&sampDesc, &g_pLinearMirrorSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pLinearMirrorSS, "g_pLinearMirrorSS");

	// 샘플러 순서가 "Common.hlsli"에서와 일관성 있어야 함
	g_ppSamplerStates.reserve(8);
	g_ppSamplerStates.push_back(g_pLinearWrapSS);    // s0
	g_ppSamplerStates.push_back(g_pLinearClampSS);   // s1
	g_ppSamplerStates.push_back(g_pShadowPointSS);   // s2
	g_ppSamplerStates.push_back(g_pShadowLinearSS);  // s3
	g_ppSamplerStates.push_back(g_pShadowCompareSS); // s4
	g_ppSamplerStates.push_back(g_pPointWrapSS);     // s5
	g_ppSamplerStates.push_back(g_pLinearMirrorSS);  // s6
	g_ppSamplerStates.push_back(g_pPointClampSS);    // s7
}

void InitRasterizerStates(ID3D11Device* pDevice)
{
	_ASSERT(pDevice);

	HRESULT hr = S_OK;

	// Rasterizer States
	D3D11_RASTERIZER_DESC rasterDesc = {};
	ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
	rasterDesc.FrontCounterClockwise = FALSE;
	rasterDesc.DepthClipEnable = TRUE;
	rasterDesc.MultisampleEnable = TRUE; // MSAA.
	hr = pDevice->CreateRasterizerState(&rasterDesc, &g_pSolidRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pSolidRS, "g_pSolidRS");

	// 거울에 반사되면 삼각형의 Winding이 바뀌기 때문에 CCW로 그려야함
	rasterDesc.FrontCounterClockwise = TRUE;
	hr = pDevice->CreateRasterizerState(&rasterDesc, &g_pSolidCcwRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pSolidCcwRS, "g_pSolidCcwRS");

	rasterDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;
	hr = pDevice->CreateRasterizerState(&rasterDesc, &g_pWireCcwRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pWireCcwRS, "g_pWireCcwRS");

	rasterDesc.FrontCounterClockwise = FALSE;
	hr = pDevice->CreateRasterizerState(&rasterDesc, &g_pWireRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pWireRS, "g_pWireRS");

	ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE; // 양면
	rasterDesc.FrontCounterClockwise = FALSE;
	rasterDesc.DepthClipEnable = TRUE;
	rasterDesc.MultisampleEnable = TRUE; // MSAA.
	hr = pDevice->CreateRasterizerState(&rasterDesc, &g_pSolidBothRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pSolidBothRS, "g_pSolidBothRS");

	rasterDesc.FrontCounterClockwise = TRUE;
	hr = pDevice->CreateRasterizerState(&rasterDesc, &g_pSolidBothCcwRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pSolidBothCcwRS, "g_pSolidBothCcwRS");

	rasterDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME; // 양면, Wire
	hr = pDevice->CreateRasterizerState(&rasterDesc, &g_pWireBothCcwRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pWireBothCcwRS, "g_pWireBothCcwRS");

	rasterDesc.FrontCounterClockwise = FALSE;
	hr = pDevice->CreateRasterizerState(&rasterDesc, &g_pWireBothRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pWireBothRS, "g_pWireBothRS");

	ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE;
	rasterDesc.FrontCounterClockwise = FALSE;
	rasterDesc.DepthClipEnable = FALSE;
	hr = pDevice->CreateRasterizerState(&rasterDesc, &g_pPostProcessingRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pPostProcessingRS, "g_pPostProcessingRS");
}

void InitBlendStates(ID3D11Device* pDevice)
{
	_ASSERT(pDevice);

	// "이미 그려져있는 화면"과 어떻게 섞을지를 결정.
	// Dest: 이미 그려져 있는 값들을 의미.
	// Src: 픽셀 쉐이더가 계산한 값들을 의미. (여기서는 마지막 거울)

	HRESULT hr = S_OK;

	D3D11_BLEND_DESC mirrorBlendDesc = {};
	ZeroMemory(&mirrorBlendDesc, sizeof(mirrorBlendDesc));
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
	hr = pDevice->CreateBlendState(&mirrorBlendDesc, &g_pMirrorBS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pMirrorBS, "g_pMirrorBS");

	D3D11_BLEND_DESC blendDesc = {};
	ZeroMemory(&blendDesc, sizeof(blendDesc));
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
	hr = pDevice->CreateBlendState(&blendDesc, &g_pAccumulateBS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pAccumulateBS, "g_pAccumulateBS");

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
	hr = pDevice->CreateBlendState(&blendDesc, &g_pAlphaBS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pAlphaBS, "g_pAlphaBS");
}

void InitDepthStencilStates(ID3D11Device* pDevice)
{
	_ASSERT(pDevice);

	// D3D11_DEPTH_STENCIL_DESC 옵션 정리.
	// https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_depth_stencil_desc
	// StencilRead/WriteMask: 예) uint8 중 어떤 비트를 사용할지.

	// D3D11_DEPTH_STENCILOP_DESC 옵션 정리.
	// https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_depth_stencilop_desc
	// StencilPassOp : 둘 다 pass일 때 할 일.
	// StencilDepthFailOp : Stencil pass, Depth fail 일 때 할 일.
	// StencilFailOp : 둘 다 fail 일 때 할 일.

	HRESULT hr = S_OK;

	// m_drawDSS: 기본 DSS
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	ZeroMemory(&dsDesc, sizeof(dsDesc));
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
	hr = pDevice->CreateDepthStencilState(&dsDesc, &g_pDrawDSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pDrawDSS, "g_pDrawDSS");

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
	hr = pDevice->CreateDepthStencilState(&dsDesc, &g_pMaskDSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pMaskDSS, "g_pMaskDSS");

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
	hr = pDevice->CreateDepthStencilState(&dsDesc, &g_pDrawMaskedDSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pDrawMaskedDSS, "g_pDrawMaskedDSS");
}

void InitShaders(ID3D11Device* pDevice)
{
	_ASSERT(pDevice);

	HRESULT hr = S_OK;

	// Shaders, InputLayouts
	const D3D11_INPUT_ELEMENT_DESC pBASIC_IEs[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	const D3D11_INPUT_ELEMENT_DESC pSKINNED_IEs[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"BLENDWEIGHT", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 76, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"BLENDINDICES", 1, DXGI_FORMAT_R8G8B8A8_UINT, 0, 80, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	const D3D11_INPUT_ELEMENT_DESC pSAMPLING_IEs[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 + 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	const D3D11_INPUT_ELEMENT_DESC pSKYBOX_IEs[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	const D3D11_INPUT_ELEMENT_DESC pGRASS_IEs[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},

		// 행렬 하나는 4x4라서 Element 4개 사용 (쉐이더에서는 행렬 하나).
		{"WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1}, // 마지막 1은 instance step
		{"WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1}, // 마지막 1은 instance step
		{"WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1}, // 마지막 1은 instance step
		{"WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48,  D3D11_INPUT_PER_INSTANCE_DATA, 1}, // 마지막 1은 instance step
		{"COLOR", 0, DXGI_FORMAT_R32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1}
	};
	const D3D11_INPUT_ELEMENT_DESC pBILLBOARD_IEs[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	const D3D_SHADER_MACRO pSKINNED_MACRO[] =
	{
		{"SKINNED", "1"}, { NULL, NULL }
	};
	UINT numBasicIEs = _countof(pBASIC_IEs);
	UINT numSkinnedIEs = _countof(pSKINNED_IEs);
	UINT numSamplingIEs = _countof(pSAMPLING_IEs);
	UINT numSkyboxIEs = _countof(pSKYBOX_IEs);
	UINT numGrassIEs = _countof(pGRASS_IEs);
	UINT numBillboardIEs = _countof(pBILLBOARD_IEs);

	hr = CreateVertexShaderAndInputLayout(pDevice, L"./Shaders/BasicVS.hlsl", pBASIC_IEs, numBasicIEs, nullptr,
										  &g_pBasicVS, &g_pBasicIL);
	BREAK_IF_FAILED(hr);

	hr = CreateVertexShaderAndInputLayout(pDevice, L"./Shaders/BasicVS.hlsl", pSKINNED_IEs, numSkinnedIEs, pSKINNED_MACRO,
										  &g_pSkinnedVS, &g_pSkinnedIL);
	BREAK_IF_FAILED(hr);

	hr = CreateVertexShaderAndInputLayout(pDevice, L"./Shaders/NormalVS.hlsl", pBASIC_IEs, numBasicIEs, nullptr,
										  &g_pNormalVS, &g_pBasicIL);
	BREAK_IF_FAILED(hr);

	hr = CreateVertexShaderAndInputLayout(pDevice, L"./Shaders/SamplingVS.hlsl", pSAMPLING_IEs, numSamplingIEs, nullptr,
										  &g_pSamplingVS, &g_pSamplingIL);
	BREAK_IF_FAILED(hr);

	hr = CreateVertexShaderAndInputLayout(pDevice, L"./Shaders/SkyboxVS.hlsl", pSKYBOX_IEs, numSkyboxIEs, nullptr,
										  &g_pSkyboxVS, &g_pSkyboxIL);
	BREAK_IF_FAILED(hr);

	hr = CreateVertexShaderAndInputLayout(pDevice, L"./Shaders/DepthOnlyVS.hlsl", pBASIC_IEs, numBasicIEs, nullptr,
										  &g_pDepthOnlyVS, &g_pSkyboxIL);
	BREAK_IF_FAILED(hr);

	hr = CreateVertexShaderAndInputLayout(pDevice, L"./Shaders/DepthOnlyVS.hlsl", pSKINNED_IEs, numSkinnedIEs, pSKINNED_MACRO,
										  &g_pDepthOnlySkinnedVS, &g_pSkinnedIL);
	BREAK_IF_FAILED(hr);

	hr = CreateVertexShaderAndInputLayout(pDevice, L"./Shaders/GrassVS.hlsl", pGRASS_IEs, numGrassIEs, nullptr,
										  &g_pGrassVS, &g_pGrassIL);
	BREAK_IF_FAILED(hr);

	hr = CreateVertexShaderAndInputLayout(pDevice, L"./Shaders/BillboardVS.hlsl", pBILLBOARD_IEs, numBillboardIEs, nullptr,
										  &g_pBillboardVS, &g_pBillboardIL);
	BREAK_IF_FAILED(hr);

	hr = CreateVertexShaderAndInputLayout(pDevice, L"./Shaders/GBufferVS.hlsl", pBASIC_IEs, numBasicIEs, nullptr,
										  &g_pGBufferVS, &g_pBasicIL);
	BREAK_IF_FAILED(hr);

	hr = CreateVertexShaderAndInputLayout(pDevice, L"./Shaders/GBufferVS.hlsl", pSKINNED_IEs, numSkinnedIEs, pSKINNED_MACRO,
										  &g_pGBufferSkinnedVS, &g_pSkinnedIL);
	BREAK_IF_FAILED(hr);

	hr = CreatePixelShader(pDevice, L"./Shaders/BasicPS.hlsl", &g_pBasicPS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/NormalPS.hlsl", &g_pNormalPS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/SkyboxPS.hlsl", &g_pSkyboxPS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/CombinePS.hlsl", &g_pCombinePS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/BloomDownPS.hlsl", &g_pBloomDownPS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/BloomUpPS.hlsl", &g_pBloomUpPS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/DepthOnlyPS.hlsl", &g_pDepthOnlyPS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/PostEffectPS.hlsl", &g_pPostEffectsPS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/ColorPS.hlsl", &g_pColorPS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/GrassPS.hlsl", &g_pGrassPS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/OceanPS.hlsl", &g_pOceanPS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/ExplosionPS.hlsl", &g_pExplosionPS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/VolumetricFirePS.hlsl", &g_pVolumetricFirePS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/GBufferPS.hlsl", &g_pGBufferPS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/DeferredLightingPS.hlsl", &g_pDeferredLightingPS);
	BREAK_IF_FAILED(hr);

	hr = CreateGeometryShader(pDevice, L"./Shaders/NormalGS.hlsl", &g_pNormalGS);
	BREAK_IF_FAILED(hr);
	hr = CreateGeometryShader(pDevice, L"./Shaders/BillboardGS.hlsl", &g_pBillboardGS);
	BREAK_IF_FAILED(hr);

	hr = CreateVertexShaderAndInputLayout(pDevice, L"./Shaders/DepthOnlyCubeVS.hlsl", pBASIC_IEs, numBasicIEs, nullptr,
										  &g_pDepthOnlyCubeVS, &g_pSkyboxIL);
	BREAK_IF_FAILED(hr);
	hr = CreateVertexShaderAndInputLayout(pDevice, L"./Shaders/DepthOnlyCubeVS.hlsl", pSKINNED_IEs, numSkinnedIEs, pSKINNED_MACRO,
										  &g_pDepthOnlyCubeSkinnedVS, &g_pSkinnedIL);
	BREAK_IF_FAILED(hr);
	hr = CreateVertexShaderAndInputLayout(pDevice, L"./Shaders/DepthOnlyCascadeVS.hlsl", pBASIC_IEs, numBasicIEs, nullptr,
										  &g_pDepthOnlyCascadeVS, &g_pSkyboxIL);
	BREAK_IF_FAILED(hr);
	hr = CreateVertexShaderAndInputLayout(pDevice, L"./Shaders/DepthONlyCascadeVS.hlsl", pSKINNED_IEs, numSkinnedIEs, pSKINNED_MACRO,
										  &g_pDepthOnlyCascadeSkinnedVS, &g_pSkinnedIL);
	BREAK_IF_FAILED(hr);
	hr = CreateGeometryShader(pDevice, L"./Shaders/DepthOnlyCubeGS.hlsl", &g_pDepthOnlyCubeGS);
	BREAK_IF_FAILED(hr);
	hr = CreateGeometryShader(pDevice, L"./Shaders/DepthOnlyCascadeGS.hlsl", &g_pDepthOnlyCascadeGS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/DepthOnlyCubePS.hlsl", &g_pDepthOnlyCubePS);
	BREAK_IF_FAILED(hr);
	hr = CreatePixelShader(pDevice, L"./Shaders/DepthOnlyCascadePS.hlsl", &g_pDepthOnlyCascadePS);
	BREAK_IF_FAILED(hr);
}

void InitPipelineStates(ID3D11Device* pDevice)
{
	_ASSERT(pDevice);

	// g_DefaultSolidPSO;
	g_DefaultSolidPSO.pVertexShader = g_pBasicVS;
	g_DefaultSolidPSO.pInputLayout = g_pBasicIL;
	g_DefaultSolidPSO.pPixelShader = g_pBasicPS;
	g_DefaultSolidPSO.pRasterizerState = g_pSolidRS;
	g_DefaultSolidPSO.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// Skinned mesh solid
	g_SkinnedSolidPSO = g_DefaultSolidPSO;
	g_SkinnedSolidPSO.pVertexShader = g_pSkinnedVS;
	g_SkinnedSolidPSO.pInputLayout = g_pSkinnedIL;

	// g_DefaultWirePSO
	g_DefaultWirePSO = g_DefaultSolidPSO;
	g_DefaultWirePSO.pRasterizerState = g_pWireRS;

	// Skinned mesh wire
	g_SkinnedWirePSO = g_SkinnedSolidPSO;
	g_SkinnedWirePSO.pRasterizerState = g_pWireRS;

	// stencilMarkPSO;
	g_StencilMaskPSO = g_DefaultSolidPSO;
	g_StencilMaskPSO.pDepthStencilState = g_pMaskDSS;
	g_StencilMaskPSO.StencilRef = 1;
	g_StencilMaskPSO.pVertexShader = g_pDepthOnlyVS;
	g_StencilMaskPSO.pPixelShader = g_pDepthOnlyPS;

	// g_ReflectSolidPSO: 반사되면 Winding 반대
	g_ReflectSolidPSO = g_DefaultSolidPSO;
	g_ReflectSolidPSO.pDepthStencilState = g_pDrawMaskedDSS;
	g_ReflectSolidPSO.pRasterizerState = g_pSolidCcwRS; // 반시계
	g_ReflectSolidPSO.StencilRef = 1;

	g_ReflectSkinnedSolidPSO = g_ReflectSolidPSO;
	g_ReflectSkinnedSolidPSO.pVertexShader = g_pSkinnedVS;
	g_ReflectSkinnedSolidPSO.pInputLayout = g_pSkinnedIL;

	// g_ReflectWirePSO: 반사되면 Winding 반대
	g_ReflectWirePSO = g_ReflectSolidPSO;
	g_ReflectWirePSO.pRasterizerState = g_pWireCcwRS; // 반시계
	g_ReflectWirePSO.StencilRef = 1;

	g_ReflectSkinnedWirePSO = g_ReflectSkinnedSolidPSO;
	g_ReflectSkinnedWirePSO.pRasterizerState = g_pWireCcwRS; // 반시계
	g_ReflectSkinnedWirePSO.StencilRef = 1;

	// g_MirrorBlendSolidPSO;
	g_MirrorBlendSolidPSO = g_DefaultSolidPSO;
	g_MirrorBlendSolidPSO.pBlendState = g_pMirrorBS;
	g_MirrorBlendSolidPSO.pDepthStencilState = g_pDrawMaskedDSS;
	g_MirrorBlendSolidPSO.StencilRef = 1;

	// g_MirrorBlendWirePSO;
	g_MirrorBlendWirePSO = g_DefaultWirePSO;
	g_MirrorBlendWirePSO.pBlendState = g_pMirrorBS;
	g_MirrorBlendWirePSO.pDepthStencilState = g_pDrawMaskedDSS;
	g_MirrorBlendWirePSO.StencilRef = 1;

	// g_SkyboxSolidPSO
	g_SkyboxSolidPSO = g_DefaultSolidPSO;
	g_SkyboxSolidPSO.pVertexShader = g_pSkyboxVS;
	g_SkyboxSolidPSO.pPixelShader = g_pSkyboxPS;
	g_SkyboxSolidPSO.pInputLayout = g_pSkyboxIL;

	// g_SkyboxWirePSO
	g_SkyboxWirePSO = g_SkyboxSolidPSO;
	g_SkyboxWirePSO.pRasterizerState = g_pWireRS;

	// g_ReflectSkyboxSolidPSO
	g_ReflectSkyboxSolidPSO = g_SkyboxSolidPSO;
	g_ReflectSkyboxSolidPSO.pDepthStencilState = g_pDrawMaskedDSS;
	g_ReflectSkyboxSolidPSO.pRasterizerState = g_pSolidCcwRS; // 반시계
	g_ReflectSkyboxSolidPSO.StencilRef = 1;

	// g_ReflectSkyboxWirePSO
	g_ReflectSkyboxWirePSO = g_ReflectSkyboxSolidPSO;
	g_ReflectSkyboxWirePSO.pRasterizerState = g_pWireCcwRS;
	g_ReflectSkyboxWirePSO.StencilRef = 1;

	// g_NormalsPSO
	g_NormalsPSO = g_DefaultSolidPSO;
	g_NormalsPSO.pVertexShader = g_pNormalVS;
	g_NormalsPSO.pGeometryShader = g_pNormalGS;
	g_NormalsPSO.pPixelShader = g_pNormalPS;
	g_NormalsPSO.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;

	// g_DepthOnlyPSO
	g_DepthOnlyPSO = g_DefaultSolidPSO;
	g_DepthOnlyPSO.pVertexShader = g_pDepthOnlyVS;
	g_DepthOnlyPSO.pPixelShader = g_pDepthOnlyPS;

	// g_DepthOnlySkinnedPSO
	g_DepthOnlySkinnedPSO = g_DepthOnlyPSO;
	g_DepthOnlySkinnedPSO.pVertexShader = g_pDepthOnlySkinnedVS;
	g_DepthOnlySkinnedPSO.pInputLayout = g_pSkinnedIL;

	// g_DepthOnlyCubePSO
	g_DepthOnlyCubePSO = g_DepthOnlyPSO;
	g_DepthOnlyCubePSO.pVertexShader = g_pDepthOnlyCubeVS;
	g_DepthOnlyCubePSO.pGeometryShader = g_pDepthOnlyCubeGS;
	g_DepthOnlyCubePSO.pPixelShader = g_pDepthOnlyCubePS;

	// g_DepthOnlyCubeSkinnedPSO
	g_DepthOnlyCubeSkinnedPSO = g_DepthOnlyCubePSO;
	g_DepthOnlyCubeSkinnedPSO.pVertexShader = g_pDepthOnlyCubeSkinnedVS;
	g_DepthOnlyCubeSkinnedPSO.pInputLayout = g_pSkinnedIL;

	// g_DepthOnlyCascadePSO
	g_DepthOnlyCascadePSO = g_DepthOnlyPSO;
	g_DepthOnlyCascadePSO.pVertexShader = g_pDepthOnlyCascadeVS;
	g_DepthOnlyCascadePSO.pGeometryShader = g_pDepthOnlyCascadeGS;
	g_DepthOnlyCascadePSO.pPixelShader = g_pDepthOnlyCascadePS;

	// g_DepthOnlyCascadeSkinnedPSO
	g_DepthOnlyCascadeSkinnedPSO = g_DepthOnlyCascadePSO;
	g_DepthOnlyCascadeSkinnedPSO.pVertexShader = g_pDepthOnlyCascadeSkinnedVS;
	g_DepthOnlyCascadeSkinnedPSO.pInputLayout = g_pSkinnedIL;

	// g_PostEffectsPSO
	g_PostEffectsPSO.pVertexShader = g_pSamplingVS;
	g_PostEffectsPSO.pPixelShader = g_pPostEffectsPS;
	g_PostEffectsPSO.pInputLayout = g_pSamplingIL;
	g_PostEffectsPSO.pRasterizerState = g_pPostProcessingRS;

	// g_PostProcessingPSO
	g_PostProcessingPSO.pVertexShader = g_pSamplingVS;
	g_PostProcessingPSO.pPixelShader = g_pDepthOnlyPS; // dummy
	g_PostProcessingPSO.pInputLayout = g_pSamplingIL;
	g_PostProcessingPSO.pRasterizerState = g_pPostProcessingRS;

	// g_BoundingBoxPSO
	g_BoundingBoxPSO = g_DefaultWirePSO;
	g_BoundingBoxPSO.pPixelShader = g_pColorPS;
	g_BoundingBoxPSO.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

	// g_GrassSolidPSO
	g_GrassSolidPSO = g_DefaultSolidPSO;
	g_GrassSolidPSO.pVertexShader = g_pGrassVS;
	g_GrassSolidPSO.pPixelShader = g_pGrassPS;
	g_GrassSolidPSO.pInputLayout = g_pGrassIL;
	g_GrassSolidPSO.pRasterizerState = g_pSolidBothRS; // 양면
	g_GrassSolidPSO.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// g_GrassWirePSO
	g_GrassWirePSO = g_GrassSolidPSO;
	g_GrassWirePSO.pRasterizerState = g_pWireBothRS; // 양면

	// g_OceanPSO
	g_OceanPSO = g_DefaultSolidPSO;
	g_OceanPSO.pBlendState = g_pAlphaBS;
	// g_OceanPSO.pRasterizerState = g_pSolidBothRS; // 양면
	g_OceanPSO.pPixelShader = g_pOceanPS;

	// g_GBufferPSO
	g_GBufferPSO = g_DefaultSolidPSO;
	g_GBufferPSO.pVertexShader = g_pGBufferVS;
	g_GBufferPSO.pPixelShader = g_pGBufferPS;

	// g_GBufferWirePSO
	g_GBufferWirePSO = g_GBufferPSO;
	g_GBufferWirePSO.pRasterizerState = g_pWireRS;

	// g_GBufferSkinnedPSO
	g_GBufferSkinnedPSO = g_GBufferPSO;
	g_GBufferSkinnedPSO.pVertexShader = g_pGBufferSkinnedVS;
	g_GBufferSkinnedPSO.pInputLayout = g_pSkinnedIL;

	// g_GBufferSKinnedWirePSO
	g_GBufferSKinnedWirePSO = g_GBufferSkinnedPSO;
	g_GBufferSkinnedPSO.pRasterizerState = g_pWireRS;

	// g_DeferredRenderingPSO
	g_DeferredRenderingPSO = g_PostProcessingPSO;
	g_DeferredRenderingPSO.pPixelShader = g_pDeferredLightingPS;
	g_DeferredRenderingPSO.pRasterizerState = g_pSolidRS;

}

// 주의: 초기화가 느려서 필요한 경우에만 초기화하는 쉐이더
void InitVolumeShaders(ID3D11Device* pDevice)
{
	_ASSERT(pDevice);

	HRESULT hr = S_OK;
	hr = CreatePixelShader(pDevice, L"VolumeSmokePS.hlsl", &g_pVolumeSmokePS);
	BREAK_IF_FAILED(hr);

	// g_VolumeSmokePSO
	g_VolumeSmokePSO = g_DefaultSolidPSO;
	g_VolumeSmokePSO.pBlendState = g_pAlphaBS;
	g_VolumeSmokePSO.pPixelShader = g_pVolumeSmokePS;
}

void DestroyCommonStates()
{
	// Sampler States
	g_ppSamplerStates.clear();
	SAFE_RELEASE(g_pLinearWrapSS);
	SAFE_RELEASE(g_pLinearClampSS);
	SAFE_RELEASE(g_pPointClampSS);
	SAFE_RELEASE(g_pShadowPointSS);
	SAFE_RELEASE(g_pShadowLinearSS);
	SAFE_RELEASE(g_pShadowCompareSS);
	SAFE_RELEASE(g_pPointWrapSS);
	SAFE_RELEASE(g_pLinearMirrorSS);

	// Rasterizer States
	SAFE_RELEASE(g_pSolidRS);
	SAFE_RELEASE(g_pSolidCcwRS);
	SAFE_RELEASE(g_pWireRS);
	SAFE_RELEASE(g_pWireCcwRS);
	SAFE_RELEASE(g_pPostProcessingRS);
	SAFE_RELEASE(g_pSolidBothRS);
	SAFE_RELEASE(g_pWireBothRS);
	SAFE_RELEASE(g_pSolidBothCcwRS);
	SAFE_RELEASE(g_pWireBothCcwRS);

	// Depth Stencil States
	SAFE_RELEASE(g_pDrawDSS);
	SAFE_RELEASE(g_pMaskDSS);
	SAFE_RELEASE(g_pDrawMaskedDSS);

	// Blend States
	SAFE_RELEASE(g_pMirrorBS);
	SAFE_RELEASE(g_pAccumulateBS);
	SAFE_RELEASE(g_pAlphaBS);

	// Shaders
	SAFE_RELEASE(g_pBasicVS);
	SAFE_RELEASE(g_pSkinnedVS);
	SAFE_RELEASE(g_pSkyboxVS);
	SAFE_RELEASE(g_pSamplingVS);
	SAFE_RELEASE(g_pNormalVS);
	SAFE_RELEASE(g_pDepthOnlyVS);
	SAFE_RELEASE(g_pDepthOnlySkinnedVS);
	SAFE_RELEASE(g_pGrassVS);
	SAFE_RELEASE(g_pBillboardVS);
	SAFE_RELEASE(g_pGBufferVS);
	SAFE_RELEASE(g_pGBufferSkinnedVS);

	SAFE_RELEASE(g_pBasicPS);
	SAFE_RELEASE(g_pSkyboxPS);
	SAFE_RELEASE(g_pCombinePS);
	SAFE_RELEASE(g_pBloomDownPS);
	SAFE_RELEASE(g_pBloomUpPS);
	SAFE_RELEASE(g_pNormalPS);
	SAFE_RELEASE(g_pDepthOnlyPS);
	SAFE_RELEASE(g_pPostEffectsPS);
	SAFE_RELEASE(g_pVolumeSmokePS);
	SAFE_RELEASE(g_pColorPS);
	SAFE_RELEASE(g_pGrassPS);
	SAFE_RELEASE(g_pOceanPS);
	SAFE_RELEASE(g_pVolumetricFirePS);
	SAFE_RELEASE(g_pExplosionPS);
	SAFE_RELEASE(g_pGBufferPS);
	SAFE_RELEASE(g_pDeferredLightingPS);

	SAFE_RELEASE(g_pNormalGS);
	SAFE_RELEASE(g_pBillboardGS);

	// Input Layouts
	SAFE_RELEASE(g_pBasicIL);
	SAFE_RELEASE(g_pSkinnedIL);
	SAFE_RELEASE(g_pSamplingIL);
	SAFE_RELEASE(g_pSkyboxIL);
	SAFE_RELEASE(g_pPostProcessingIL);
	SAFE_RELEASE(g_pGrassIL);
	SAFE_RELEASE(g_pBillboardIL);

	SAFE_RELEASE(g_pDepthOnlyCubeVS);
	SAFE_RELEASE(g_pDepthOnlyCubeSkinnedVS);
	SAFE_RELEASE(g_pDepthOnlyCascadeVS);
	SAFE_RELEASE(g_pDepthOnlyCascadeSkinnedVS);
	SAFE_RELEASE(g_pDepthOnlyCubeGS);
	SAFE_RELEASE(g_pDepthOnlyCascadeGS);
	SAFE_RELEASE(g_pDepthOnlyCubePS);
	SAFE_RELEASE(g_pDepthOnlyCascadePS);
}
