#pragma once

#include "GraphicsUtils.h"
#include "PipelineState.h"

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/GraphicsCommon.h

namespace Graphics
{
	// Samplers
	extern ID3D11SamplerState* g_pLinearWrapSS;
	extern ID3D11SamplerState* g_pLinearClampSS;
	extern ID3D11SamplerState* g_pPointClampSS;
	extern ID3D11SamplerState* g_pShadowPointSS;
	extern ID3D11SamplerState* g_pShadowCompareSS;
	extern ID3D11SamplerState* g_pPointWrapSS;
	extern ID3D11SamplerState* g_pLinearMirrorSS;
	extern std::vector<ID3D11SamplerState*> g_ppSamplerStates;

	// Rasterizer States
	// CCW: Counter-Clockwise (반시계 방향을 의미합니다.)
	extern ID3D11RasterizerState* g_pSolidRS; // front only
	extern ID3D11RasterizerState* g_pSolidCcwRS;
	extern ID3D11RasterizerState* g_pWireRS;
	extern ID3D11RasterizerState* g_pWireCcwRS;
	extern ID3D11RasterizerState* g_pPostProcessingRS;
	extern ID3D11RasterizerState* g_pSolidBothRS; // front and back
	extern ID3D11RasterizerState* g_pWireBothRS;
	extern ID3D11RasterizerState* g_pSolidBothCcwRS;
	extern ID3D11RasterizerState* g_pWireBothCcwRS;

	// Depth Stencil States
	extern ID3D11DepthStencilState* g_pDrawDSS; // 일반적으로 그리기
	extern ID3D11DepthStencilState* g_pMaskDSS; // 스텐실버퍼에 표시
	extern ID3D11DepthStencilState* g_pDrawMaskedDSS; // 스텐실 표시된 곳만

	// Shaders
	extern ID3D11VertexShader* g_pBasicVS;
	extern ID3D11VertexShader* g_pSkinnedVS; // g_pBasicVS.hlsl에 SKINNED 매크로
	extern ID3D11VertexShader* g_pSkyboxVS;
	extern ID3D11VertexShader* g_pSamplingVS;
	extern ID3D11VertexShader* g_pNormalVS;
	extern ID3D11VertexShader* g_pDepthOnlyVS;
	extern ID3D11VertexShader* g_pDepthOnlySkinnedVS;
	extern ID3D11VertexShader* g_pGrassVS;
	extern ID3D11VertexShader* g_pBillboardVS;
	extern ID3D11PixelShader* g_pBasicPS;
	extern ID3D11PixelShader* g_pSkyboxPS;
	extern ID3D11PixelShader* g_pCombinePS;
	extern ID3D11PixelShader* g_pBloomDownPS;
	extern ID3D11PixelShader* g_pBloomUpPS;
	extern ID3D11PixelShader* g_pNormalPS;
	extern ID3D11PixelShader* g_pDepthOnlyPS;
	extern ID3D11PixelShader* g_pPostEffectsPS;
	extern ID3D11PixelShader* g_pVolumeSmokePS;
	extern ID3D11PixelShader* g_pGrassPS;
	extern ID3D11PixelShader* g_pOceanPS;
	extern ID3D11PixelShader* g_pVolumetricFirePS;
	extern ID3D11PixelShader* g_pExplosionPS;
	extern ID3D11GeometryShader* g_pNormalGS;
	extern ID3D11GeometryShader* g_pBillboardGS;

	// Input Layouts
	extern ID3D11InputLayout* g_pBasicIL;
	extern ID3D11InputLayout* g_pSkinnedIL;
	extern ID3D11InputLayout* g_pSamplingIL;
	extern ID3D11InputLayout* g_pSkyboxIL;
	extern ID3D11InputLayout* g_pPostProcessingIL;
	extern ID3D11InputLayout* g_pGrassIL;
	extern ID3D11InputLayout* g_pBillboardIL;

	// Blend States
	extern ID3D11BlendState* g_pMirrorBS;
	extern ID3D11BlendState* g_pAccumulateBS;
	extern ID3D11BlendState* g_pAlphaBS;

	// Graphics Pipeline States
	extern Graphics::GraphicsPSO g_DefaultSolidPSO;
	extern Graphics::GraphicsPSO g_SkinnedSolidPSO;
	extern Graphics::GraphicsPSO g_DefaultWirePSO;
	extern Graphics::GraphicsPSO g_SkinnedWirePSO;
	extern Graphics::GraphicsPSO g_StencilMaskPSO;
	extern Graphics::GraphicsPSO g_ReflectSolidPSO;
	extern Graphics::GraphicsPSO g_ReflectSkinnedSolidPSO;
	extern Graphics::GraphicsPSO g_ReflectWirePSO;
	extern Graphics::GraphicsPSO g_ReflectSkinnedWirePSO;
	extern Graphics::GraphicsPSO g_MirrorBlendSolidPSO;
	extern Graphics::GraphicsPSO g_MirrorBlendWirePSO;
	extern Graphics::GraphicsPSO g_SkyboxSolidPSO;
	extern Graphics::GraphicsPSO g_SkyboxWirePSO;
	extern Graphics::GraphicsPSO g_ReflectSkyboxSolidPSO;
	extern Graphics::GraphicsPSO g_ReflectSkyboxWirePSO;
	extern Graphics::GraphicsPSO g_NormalsPSO;
	extern Graphics::GraphicsPSO g_DepthOnlyPSO;
	extern Graphics::GraphicsPSO g_DepthOnlySkinnedPSO;
	extern Graphics::GraphicsPSO g_PostEffectsPSO;
	extern Graphics::GraphicsPSO g_PostProcessingPSO;
	extern Graphics::GraphicsPSO g_BoundingBoxPSO;
	extern Graphics::GraphicsPSO g_GrassSolidPSO;
	extern Graphics::GraphicsPSO g_GrassWirePSO;
	extern Graphics::GraphicsPSO g_OceanPSO;

	// 주의: 초기화가 느려서 필요한 경우에만 초기화.
	extern Graphics::GraphicsPSO g_VolumeSmokePSO;

	void InitCommonStates(ID3D11Device* pDevice);

	// 내부적으로 InitCommonStates()에서 사용.
	void InitSamplers(ID3D11Device* pDevice);
	void InitRasterizerStates(ID3D11Device* pDevice);
	void InitBlendStates(ID3D11Device* pDevice);
	void InitDepthStencilStates(ID3D11Device* pDevice);
	void InitPipelineStates(ID3D11Device* pDevice);
	void InitShaders(ID3D11Device* pDevice);

	// 주의: 초기화가 느려서 필요한 경우에만 초기화.
	void InitVolumeShaders(ID3D11Device* pDevice);

	void DestroyCommonStates();
}
