#include "../Common.h"
#include "GraphicsCommon.h"

namespace Graphics
{
	// Sampler States
	ID3D11SamplerState* g_pLinearWrapSS = nullptr;
	ID3D11SamplerState* g_pLinearClampSS = nullptr;
	ID3D11SamplerState* g_pPointClampSS = nullptr;
	ID3D11SamplerState* g_pShadowPointSS = nullptr;
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
	ID3D11DepthStencilState* g_pDrawDSS = nullptr;       // �Ϲ������� �׸���
	ID3D11DepthStencilState* g_pMaskDSS = nullptr;       // ���ٽǹ��ۿ� ǥ��
	ID3D11DepthStencilState* g_pDrawMaskedDSS = nullptr; // ���ٽ� ǥ�õ� ����

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

	ID3D11GeometryShader* g_pNormalGS = nullptr;
	ID3D11GeometryShader* g_pBillboardGS = nullptr;

	// Input Layouts
	ID3D11InputLayout* g_pBasicIL = nullptr;
	ID3D11InputLayout* g_pSkinnedIL = nullptr;
	ID3D11InputLayout* g_pSamplingIL = nullptr;
	ID3D11InputLayout* g_pSkyboxIL = nullptr;
	ID3D11InputLayout* g_pPostProcessingIL = nullptr;
	ID3D11InputLayout* g_pGrassIL = nullptr;     // PER_INSTANCE ���
	ID3D11InputLayout* g_pBillboardIL = nullptr; // PER_INSTANCE ���

	// Graphics Pipeline States
	Graphics::GraphicsPSO g_DefaultSolidPSO;
	Graphics::GraphicsPSO g_SkinnedSolidPSO;
	Graphics::GraphicsPSO g_DefaultWirePSO;
	Graphics::GraphicsPSO g_SkinnedWirePSO;
	Graphics::GraphicsPSO g_StencilMaskPSO;
	Graphics::GraphicsPSO g_ReflectSolidPSO;
	Graphics::GraphicsPSO g_ReflectSkinnedSolidPSO;
	Graphics::GraphicsPSO g_ReflectWirePSO;
	Graphics::GraphicsPSO g_ReflectSkinnedWirePSO;
	Graphics::GraphicsPSO g_MirrorBlendSolidPSO;
	Graphics::GraphicsPSO g_MirrorBlendWirePSO;
	Graphics::GraphicsPSO g_SkyboxSolidPSO;
	Graphics::GraphicsPSO g_SkyboxWirePSO;
	Graphics::GraphicsPSO g_ReflectSkyboxSolidPSO;
	Graphics::GraphicsPSO g_ReflectSkyboxWirePSO;
	Graphics::GraphicsPSO g_NormalsPSO;
	Graphics::GraphicsPSO g_DepthOnlyPSO;
	Graphics::GraphicsPSO g_DepthOnlySkinnedPSO;
	Graphics::GraphicsPSO g_PostEffectsPSO;
	Graphics::GraphicsPSO g_PostProcessingPSO;
	Graphics::GraphicsPSO g_BoundingBoxPSO;
	Graphics::GraphicsPSO g_GrassSolidPSO;
	Graphics::GraphicsPSO g_GrassWirePSO;
	Graphics::GraphicsPSO g_OceanPSO;

	// ����: �ʱ�ȭ�� ������ �ʿ��� ��쿡�� �ʱ�ȭ
	Graphics::GraphicsPSO g_VolumeSmokePSO;

	// Compute Pipeline States

}

void Graphics::InitCommonStates(ID3D11Device* pDevice)
{
	InitShaders(pDevice);
	InitSamplers(pDevice);
	InitRasterizerStates(pDevice);
	InitBlendStates(pDevice);
	InitDepthStencilStates(pDevice);
	InitPipelineStates(pDevice);
}

void Graphics::InitSamplers(ID3D11Device* pDevice)
{
	HRESULT hr = S_OK;

	D3D11_SAMPLER_DESC sampDesc;
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
	sampDesc.BorderColor[0] = 1.0f; // ū Z��
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hr = pDevice->CreateSamplerState(&sampDesc, &g_pShadowPointSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pShadowPointSS, "g_pShadowPointSS");

	// shadowCompareSS, ���̴� �ȿ����� SamplerComparisonState
	// Filter = "_COMPARISON_" ����
	// https://www.gamedev.net/forums/topic/670575-uploading-samplercomparisonstate-in-hlsl/
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.BorderColor[0] = 100.0f; // ū Z��
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

	// ���÷� ������ "Common.hlsli"������ �ϰ��� �־�� ��
	g_ppSamplerStates.push_back(g_pLinearWrapSS);    // s0
	g_ppSamplerStates.push_back(g_pLinearClampSS);   // s1
	g_ppSamplerStates.push_back(g_pShadowPointSS);   // s2
	g_ppSamplerStates.push_back(g_pShadowCompareSS); // s3
	g_ppSamplerStates.push_back(g_pPointWrapSS);     // s4
	g_ppSamplerStates.push_back(g_pLinearMirrorSS);  // s5
	g_ppSamplerStates.push_back(g_pPointClampSS);    // s6
}

void Graphics::InitRasterizerStates(ID3D11Device* pDevice)
{
	HRESULT hr = S_OK;

	// Rasterizer States
	D3D11_RASTERIZER_DESC rasterDesc;
	ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
	rasterDesc.FrontCounterClockwise = FALSE;
	rasterDesc.DepthClipEnable = TRUE;
	rasterDesc.MultisampleEnable = TRUE; // MSAA.
	hr = pDevice->CreateRasterizerState(&rasterDesc, &g_pSolidRS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pSolidRS, "g_pSolidRS");

	// �ſ￡ �ݻ�Ǹ� �ﰢ���� Winding�� �ٲ�� ������ CCW�� �׷�����
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
	rasterDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE; // ���
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

	rasterDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME; // ���, Wire
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

void Graphics::InitBlendStates(ID3D11Device* pDevice)
{
	// "�̹� �׷����ִ� ȭ��"�� ��� �������� ����.
	// Dest: �̹� �׷��� �ִ� ������ �ǹ�.
	// Src: �ȼ� ���̴��� ����� ������ �ǹ�. (���⼭�� ������ �ſ�)

	HRESULT hr = S_OK;

	D3D11_BLEND_DESC mirrorBlendDesc;
	ZeroMemory(&mirrorBlendDesc, sizeof(mirrorBlendDesc));
	mirrorBlendDesc.AlphaToCoverageEnable = TRUE;
	mirrorBlendDesc.IndependentBlendEnable = FALSE;
	// ���� RenderTarget�� ���ؼ� ���� (�ִ� 8��).
	mirrorBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	mirrorBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_BLEND_FACTOR;
	mirrorBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_BLEND_FACTOR;
	mirrorBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	mirrorBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	mirrorBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	mirrorBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	// �ʿ��ϸ� RGBA ������ ���ؼ��� ���� ����.
	mirrorBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = pDevice->CreateBlendState(&mirrorBlendDesc, &g_pMirrorBS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pMirrorBS, "g_pMirrorBS");

	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable = TRUE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_BLEND_FACTOR;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_BLEND_FACTOR; // INV �ƴ�
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = pDevice->CreateBlendState(&blendDesc, &g_pAccumulateBS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pAccumulateBS, "g_pAccumulateBS");

	// Dst: ���� �����, Src: ���� �ȼ� ���̴����� ���.
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable = FALSE; // <- ����: FALSE
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

void Graphics::InitDepthStencilStates(ID3D11Device* pDevice)
{
	// D3D11_DEPTH_STENCIL_DESC �ɼ� ����.
	// https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_depth_stencil_desc
	// StencilRead/WriteMask: ��) uint8 �� � ��Ʈ�� �������.

	// D3D11_DEPTH_STENCILOP_DESC �ɼ� ����.
	// https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_depth_stencilop_desc
	// StencilPassOp : �� �� pass�� �� �� ��.
	// StencilDepthFailOp : Stencil pass, Depth fail �� �� �� ��.
	// StencilFailOp : �� �� fail �� �� �� ��.

	HRESULT hr = S_OK;

	// m_drawDSS: �⺻ DSS
	D3D11_DEPTH_STENCIL_DESC dsDesc;
	ZeroMemory(&dsDesc, sizeof(dsDesc));
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dsDesc.StencilEnable = FALSE; // Stencil ���ʿ�.
	dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	// �ո鿡 ���ؼ� ��� �۵����� ����.
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// �޸鿡 ���� ��� �۵����� ���� (�޸鵵 �׸� ���).
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	hr = pDevice->CreateDepthStencilState(&dsDesc, &g_pDrawDSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pDrawDSS, "g_pDrawDSS");

	// Stencil�� 1�� ǥ�����ִ� DSS.
	dsDesc.DepthEnable = TRUE; // �̹� �׷��� ��ü ����.
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dsDesc.StencilEnable = TRUE;    // Stencil �ʼ�.
	dsDesc.StencilReadMask = 0xFF;  // ��� ��Ʈ �� ���.
	dsDesc.StencilWriteMask = 0xFF; // ��� ��Ʈ �� ���.
	// �ո鿡 ���ؼ� ��� �۵����� ����.
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	hr = pDevice->CreateDepthStencilState(&dsDesc, &g_pMaskDSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pMaskDSS, "g_pMaskDSS");

	// Stencil�� 1�� ǥ��� ��쿡"��" �׸��� DSS.
	// DepthBuffer�� �ʱ�ȭ�� ���·� ����.
	// D3D11_COMPARISON_EQUAL �̹� 1�� ǥ��� ��쿡�� �׸���.
	// OMSetDepthStencilState(..., 1); <- ������ 1.
	dsDesc.DepthEnable = TRUE;   // �ſ� ���� �ٽ� �׸��� �ʿ�.
	dsDesc.StencilEnable = TRUE; // Stencil ���.
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // <- ����
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	hr = pDevice->CreateDepthStencilState(&dsDesc, &g_pDrawMaskedDSS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pDrawMaskedDSS, "g_pDrawMaskedDSS");
}

void Graphics::InitShaders(ID3D11Device* pDevice)
{
	HRESULT hr = S_OK;

	// Shaders, InputLayouts
	const D3D11_INPUT_ELEMENT_DESC pBASIC_IEs[]  =
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
	const D3D11_INPUT_ELEMENT_DESC pSKYBOX_IEs[]  =
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

		// ��� �ϳ��� 4x4�� Element 4�� ��� (���̴������� ��� �ϳ�).
		{"WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1}, // ������ 1�� instance step
		{"WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1}, // ������ 1�� instance step
		{"WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1}, // ������ 1�� instance step
		{"WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48,  D3D11_INPUT_PER_INSTANCE_DATA, 1}, // ������ 1�� instance step
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

	hr = Graphics::CreateVertexShaderAndInputLayout(pDevice,
													L"./Shaders/BasicVS.hlsl",
													pBASIC_IEs, numBasicIEs, nullptr,
													&g_pBasicVS, &g_pBasicIL);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pBasicVS, "g_pBasicVS");
	SET_DEBUG_INFO_TO_OBJECT(g_pBasicIL, "g_pBasicIL");

	hr = Graphics::CreateVertexShaderAndInputLayout(pDevice,
													L"./Shaders/BasicVS.hlsl",
													pSKINNED_IEs, numSkinnedIEs, pSKINNED_MACRO,
													&g_pSkinnedVS, &g_pSkinnedIL);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pSkinnedVS, "g_pSkinnedVS");
	SET_DEBUG_INFO_TO_OBJECT(g_pSkinnedIL, "g_pSkinnedIL");

	hr = Graphics::CreateVertexShaderAndInputLayout(pDevice,
													L"./Shaders/NormalVS.hlsl",
													pBASIC_IEs,numBasicIEs, nullptr,
													&g_pNormalVS, &g_pBasicIL);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pNormalVS, "g_pNormalVS");

	hr = Graphics::CreateVertexShaderAndInputLayout(pDevice,
													L"./Shaders/SamplingVS.hlsl",
													pSAMPLING_IEs, numSamplingIEs, nullptr,
													&g_pSamplingVS, &g_pSamplingIL);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pSamplingVS, "g_pSamplingVS");
	SET_DEBUG_INFO_TO_OBJECT(g_pSamplingIL, "g_pSamplingIL");

	hr = Graphics::CreateVertexShaderAndInputLayout(pDevice,
													L"./Shaders/SkyboxVS.hlsl",
													pSKYBOX_IEs, numSkyboxIEs, nullptr,
													&g_pSkyboxVS, &g_pSkyboxIL);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pSkyboxVS, "g_pSkyboxVS");
	SET_DEBUG_INFO_TO_OBJECT(g_pSkyboxIL, "g_pSkyboxIL");

	hr = Graphics::CreateVertexShaderAndInputLayout(pDevice,
													L"./Shaders/DepthOnlyVS.hlsl",
													pBASIC_IEs, numBasicIEs, nullptr,
													&g_pDepthOnlyVS, &g_pSkyboxIL);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pDepthOnlyVS, "g_pDepthOnlyVS");

	hr = Graphics::CreateVertexShaderAndInputLayout(pDevice,
													L"./Shaders/DepthOnlyVS.hlsl",
													pSKINNED_IEs, numSkinnedIEs, pSKINNED_MACRO,
													&g_pDepthOnlySkinnedVS, &g_pSkinnedIL);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pDepthOnlySkinnedVS, "g_pDepthOnlySkinnedVS");
	SET_DEBUG_INFO_TO_OBJECT(g_pSkinnedIL, "g_pSkinnedIL");

	hr = Graphics::CreateVertexShaderAndInputLayout(pDevice,
													L"./Shaders/GrassVS.hlsl",
													pGRASS_IEs, numGrassIEs, nullptr,
													&g_pGrassVS, &g_pGrassIL);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pGrassVS, "g_pGrassVS");
	SET_DEBUG_INFO_TO_OBJECT(g_pGrassIL, "g_pGrassIL");

	hr = Graphics::CreateVertexShaderAndInputLayout(pDevice,
													L"./Shaders/BillboardVS.hlsl",
													pBILLBOARD_IEs, numBillboardIEs, nullptr,
													&g_pBillboardVS, &g_pBillboardIL);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pBillboardVS, "g_pBillboardVS");
	SET_DEBUG_INFO_TO_OBJECT(g_pBillboardIL, "g_pBillboardIL");

	hr = Graphics::CreatePixelShader(pDevice, L"./Shaders/BasicPS.hlsl", &g_pBasicPS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pBasicPS, "g_pBasicPS");
	hr = Graphics::CreatePixelShader(pDevice, L"./Shaders/NormalPS.hlsl", &g_pNormalPS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pNormalPS, "g_pNormalPS");
	hr = Graphics::CreatePixelShader(pDevice, L"./Shaders/SkyboxPS.hlsl", &g_pSkyboxPS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pSkyboxPS, "g_pSkyboxPS");
	hr = Graphics::CreatePixelShader(pDevice, L"./Shaders/CombinePS.hlsl", &g_pCombinePS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pCombinePS, "g_pCombinePS");
	hr = Graphics::CreatePixelShader(pDevice, L"./Shaders/BloomDownPS.hlsl", &g_pBloomDownPS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pBloomDownPS, "g_pBloomDownPS");
	hr = Graphics::CreatePixelShader(pDevice, L"./Shaders/BloomUpPS.hlsl", &g_pBloomUpPS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pBloomUpPS, "g_pBloomUpPS");
	hr = Graphics::CreatePixelShader(pDevice, L"./Shaders/DepthOnlyPS.hlsl", &g_pDepthOnlyPS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pDepthOnlyPS, "g_pDepthOnlyPS");
	hr = Graphics::CreatePixelShader(pDevice, L"./Shaders/PostEffectPS.hlsl", &g_pPostEffectsPS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pPostEffectsPS, "g_pPostEffectsPS");
	hr = Graphics::CreatePixelShader(pDevice, L"./Shaders/ColorPS.hlsl", &g_pColorPS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pColorPS, "g_pColorPS");
	hr = Graphics::CreatePixelShader(pDevice, L"./Shaders/GrassPS.hlsl", &g_pGrassPS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pGrassPS, "g_pGrassPS");
	hr = Graphics::CreatePixelShader(pDevice, L"./Shaders/OceanPS.hlsl", &g_pOceanPS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pOceanPS, "g_pOceanPS");
	hr = Graphics::CreatePixelShader(pDevice, L"./Shaders/ExplosionPS.hlsl", &g_pExplosionPS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pExplosionPS, "g_pExplosionPS");
	Graphics::CreatePixelShader(pDevice, L"./Shaders/VolumetricFirePS.hlsl", &g_pVolumetricFirePS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pVolumetricFirePS, "g_pVolumetricFirePS");

	hr = Graphics::CreateGeometryShader(pDevice, L"./Shaders/NormalGS.hlsl", &g_pNormalGS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pNormalGS, "g_pNormalGS");
	hr = Graphics::CreateGeometryShader(pDevice, L"./Shaders/BillboardGS.hlsl", &g_pBillboardGS);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(g_pBillboardGS, "g_pBillboardGS");
}

void Graphics::InitPipelineStates(ID3D11Device* pDevice)
{
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

	// g_ReflectSolidPSO: �ݻ�Ǹ� Winding �ݴ�
	g_ReflectSolidPSO = g_DefaultSolidPSO;
	g_ReflectSolidPSO.pDepthStencilState = g_pDrawMaskedDSS;
	g_ReflectSolidPSO.pRasterizerState = g_pSolidCcwRS; // �ݽð�
	g_ReflectSolidPSO.StencilRef = 1;

	g_ReflectSkinnedSolidPSO = g_ReflectSolidPSO;
	g_ReflectSkinnedSolidPSO.pVertexShader = g_pSkinnedVS;
	g_ReflectSkinnedSolidPSO.pInputLayout = g_pSkinnedIL;

	// g_ReflectWirePSO: �ݻ�Ǹ� Winding �ݴ�
	g_ReflectWirePSO = g_ReflectSolidPSO;
	g_ReflectWirePSO.pRasterizerState = g_pWireCcwRS; // �ݽð�
	g_ReflectWirePSO.StencilRef = 1;

	g_ReflectSkinnedWirePSO = g_ReflectSkinnedSolidPSO;
	g_ReflectSkinnedWirePSO.pRasterizerState = g_pWireCcwRS; // �ݽð�
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
	g_ReflectSkyboxSolidPSO.pRasterizerState = g_pSolidCcwRS; // �ݽð�
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

	g_DepthOnlySkinnedPSO = g_DepthOnlyPSO;
	g_DepthOnlySkinnedPSO.pVertexShader = g_pDepthOnlySkinnedVS;
	g_DepthOnlySkinnedPSO.pInputLayout = g_pSkinnedIL;

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
	g_GrassSolidPSO.pRasterizerState = g_pSolidBothRS; // ���
	g_GrassSolidPSO.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// g_GrassWirePSO
	g_GrassWirePSO = g_GrassSolidPSO;
	g_GrassWirePSO.pRasterizerState = g_pWireBothRS; // ���

	// g_OceanPSO
	g_OceanPSO = g_DefaultSolidPSO;
	g_OceanPSO.pBlendState = g_pAlphaBS;
	// g_OceanPSO.pRasterizerState = g_pSolidBothRS; // ���
	g_OceanPSO.pPixelShader = g_pOceanPS;
}

// ����: �ʱ�ȭ�� ������ �ʿ��� ��쿡�� �ʱ�ȭ�ϴ� ���̴�
void Graphics::InitVolumeShaders(ID3D11Device* pDevice)
{
	HRESULT hr = S_OK;
	hr = Graphics::CreatePixelShader(pDevice, L"VolumeSmokePS.hlsl", &g_pVolumeSmokePS);
	BREAK_IF_FAILED(hr);

	// g_VolumeSmokePSO
	g_VolumeSmokePSO = g_DefaultSolidPSO;
	g_VolumeSmokePSO.pBlendState = g_pAlphaBS;
	g_VolumeSmokePSO.pPixelShader = g_pVolumeSmokePS;
}

void Graphics::DestroyCommonStates()
{
	// Sampler States
	g_ppSamplerStates.clear();
	SAFE_RELEASE(g_pLinearWrapSS);
	SAFE_RELEASE(g_pLinearClampSS);
	SAFE_RELEASE(g_pPointClampSS);
	SAFE_RELEASE(g_pShadowPointSS);
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
}