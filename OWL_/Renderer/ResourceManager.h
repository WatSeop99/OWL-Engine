#pragma once

#include "PipelineState.h"

class ResourceManager
{
public:
	ResourceManager() = default;
	~ResourceManager() { Cleanup(); };

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

	HRESULT CreateVertexBuffer(UINT sizePerVertex, UINT numVertex, ID3D11Buffer** ppOutVertexBuffer, void* pInitData);
	HRESULT CreateIndexBuffer(UINT sizePerIndex, UINT numIndex, ID3D11Buffer** ppOutIndexBuffer, void* pInitData);

	HRESULT CreateTextureFromFile(const WCHAR* pszFileName, ID3D11Texture2D** ppOutTexture, D3D11_TEXTURE2D_DESC* pOutDesc, bool bUseSRGB);
	HRESULT CreateTextureCubeFromFile(const WCHAR* pszFileName, ID3D11Texture2D** ppOutTexture, D3D11_TEXTURE2D_DESC* pOutDesc);
	HRESULT CreateTexture(const D3D11_TEXTURE2D_DESC& TEXTURE_DESC, void* pInitData, ID3D11Texture2D** ppOutTexture);

	void Cleanup();

protected:
	void initSamplers();
	void initRasterizerStates();
	void initBlendStates();
	void initDepthStencilStates();
	void initShaders();
	void initPipelineStates();

	// volume shader 제외.

	HRESULT createVertexShaderAndInputLayout(const WCHAR* pszFileName, const D3D11_INPUT_ELEMENT_DESC* pINPUT_ELEMENTS, const UINT ELEMENT_SIZE, const D3D_SHADER_MACRO* pSHADER_MACROS, ID3D11VertexShader** ppOutVertexShader, ID3D11InputLayout** ppOutInputLayout);
	HRESULT createHullShader(const WCHAR* pszFileName, ID3D11HullShader** ppOutHullShader);
	HRESULT createDomainShader(const WCHAR* pszFileName, ID3D11DomainShader** ppOutDomainShader);
	HRESULT createGeometryShader(const WCHAR* pszFileName, ID3D11GeometryShader** ppOutGeometryShader);
	HRESULT createPixelShader(const WCHAR* pszFileName, ID3D11PixelShader** ppOutPixelShader);
	HRESULT createComputeShader(const WCHAR* pszFileName, ID3D11ComputeShader** ppOutComputeShader);

public:
	ID3D11SamplerState* pLinearWrapSS = nullptr;
	ID3D11SamplerState* pLinearClampSS = nullptr;
	ID3D11SamplerState* pPointClampSS = nullptr;
	ID3D11SamplerState* pShadowPointSS = nullptr;
	ID3D11SamplerState* pShadowLinearSS = nullptr;
	ID3D11SamplerState* pShadowCompareSS = nullptr;
	ID3D11SamplerState* pPointWrapSS = nullptr;
	ID3D11SamplerState* pLinearMirrorSS = nullptr;
	std::vector<ID3D11SamplerState*> SamplerStates;

	ID3D11RasterizerState* pSolidRS = nullptr; // front only
	ID3D11RasterizerState* pSolidCcwRS = nullptr;
	ID3D11RasterizerState* pWireRS = nullptr;
	ID3D11RasterizerState* pWireCcwRS = nullptr;
	ID3D11RasterizerState* pPostProcessingRS = nullptr;
	ID3D11RasterizerState* pSolidBothRS = nullptr; // front and back
	ID3D11RasterizerState* pWireBothRS = nullptr;
	ID3D11RasterizerState* pSolidBothCcwRS = nullptr;
	ID3D11RasterizerState* pWireBothCcwRS = nullptr;

	ID3D11DepthStencilState* pDrawDSS = nullptr; // 일반적으로 그리기
	ID3D11DepthStencilState* pMaskDSS = nullptr; // 스텐실버퍼에 표시
	ID3D11DepthStencilState* pDrawMaskedDSS = nullptr; // 스텐실 표시된 곳만

	ID3D11VertexShader* pBasicVS = nullptr;
	ID3D11VertexShader* pSkinnedVS = nullptr; // g_pBasicVS.hlsl에 SKINNED 매크로
	ID3D11VertexShader* pSkyboxVS = nullptr;
	ID3D11VertexShader* pSamplingVS = nullptr;
	ID3D11VertexShader* pNormalVS = nullptr;
	ID3D11VertexShader* pDepthOnlyVS = nullptr;
	ID3D11VertexShader* pDepthOnlySkinnedVS = nullptr;
	ID3D11VertexShader* pDepthOnlyCubeVS = nullptr;
	ID3D11VertexShader* pDepthOnlyCubeSkinnedVS = nullptr;
	ID3D11VertexShader* pDepthOnlyCascadeVS = nullptr;
	ID3D11VertexShader* pDepthOnlyCascadeSkinnedVS = nullptr;
	ID3D11VertexShader* pGrassVS = nullptr;
	ID3D11VertexShader* pBillboardVS = nullptr;
	ID3D11VertexShader* pGBufferVS = nullptr;
	ID3D11VertexShader* pGBufferSkinnedVS = nullptr;
	ID3D11PixelShader* pBasicPS = nullptr;
	ID3D11PixelShader* pSkyboxPS = nullptr;
	ID3D11PixelShader* pCombinePS = nullptr;
	ID3D11PixelShader* pBloomDownPS = nullptr;
	ID3D11PixelShader* pBloomUpPS = nullptr;
	ID3D11PixelShader* pNormalPS = nullptr;
	ID3D11PixelShader* pDepthOnlyPS = nullptr;
	ID3D11PixelShader* pDepthOnlyCubePS = nullptr;
	ID3D11PixelShader* pDepthOnlyCascadePS = nullptr;
	ID3D11PixelShader* pPostEffectsPS = nullptr;
	ID3D11PixelShader* pVolumeSmokePS = nullptr;
	ID3D11PixelShader* pGrassPS = nullptr;
	ID3D11PixelShader* pOceanPS = nullptr;
	ID3D11PixelShader* pVolumetricFirePS = nullptr;
	ID3D11PixelShader* pColorPS = nullptr;
	ID3D11PixelShader* pExplosionPS = nullptr;
	ID3D11PixelShader* pGBufferPS = nullptr;
	ID3D11PixelShader* pDeferredLightingPS = nullptr;
	ID3D11GeometryShader* pNormalGS = nullptr;
	ID3D11GeometryShader* pBillboardGS = nullptr;
	ID3D11GeometryShader* pDepthOnlyCubeGS = nullptr;
	ID3D11GeometryShader* pDepthOnlyCascadeGS = nullptr;

	ID3D11InputLayout* pBasicIL = nullptr;
	ID3D11InputLayout* pSkinnedIL = nullptr;
	ID3D11InputLayout* pSamplingIL = nullptr;
	ID3D11InputLayout* pSkyboxIL = nullptr;
	ID3D11InputLayout* pPostProcessingIL = nullptr;
	ID3D11InputLayout* pGrassIL = nullptr;
	ID3D11InputLayout* pBillboardIL = nullptr;

	ID3D11BlendState* pMirrorBS = nullptr;
	ID3D11BlendState* pAccumulateBS = nullptr;
	ID3D11BlendState* pAlphaBS = nullptr;

	GraphicsPSO DefaultSolidPSO;
	GraphicsPSO SkinnedSolidPSO;
	GraphicsPSO DefaultWirePSO;
	GraphicsPSO SkinnedWirePSO;
	GraphicsPSO StencilMaskPSO;
	GraphicsPSO ReflectSolidPSO;
	GraphicsPSO ReflectSkinnedSolidPSO;
	GraphicsPSO ReflectWirePSO;
	GraphicsPSO ReflectSkinnedWirePSO;
	GraphicsPSO MirrorBlendSolidPSO;
	GraphicsPSO MirrorBlendWirePSO;
	GraphicsPSO SkyboxSolidPSO;
	GraphicsPSO SkyboxWirePSO;
	GraphicsPSO ReflectSkyboxSolidPSO;
	GraphicsPSO ReflectSkyboxWirePSO;
	GraphicsPSO NormalsPSO;
	GraphicsPSO DepthOnlyPSO;
	GraphicsPSO DepthOnlySkinnedPSO;
	GraphicsPSO DepthOnlyCubePSO;
	GraphicsPSO DepthOnlyCubeSkinnedPSO;
	GraphicsPSO DepthOnlyCascadePSO;
	GraphicsPSO DepthOnlyCascadeSkinnedPSO;
	GraphicsPSO PostEffectsPSO;
	GraphicsPSO PostProcessingPSO;
	GraphicsPSO BoundingBoxPSO;
	GraphicsPSO GrassSolidPSO;
	GraphicsPSO GrassWirePSO;
	GraphicsPSO OceanPSO;
	GraphicsPSO GBufferPSO;
	GraphicsPSO GBufferWirePSO;
	GraphicsPSO GBufferSkinnedPSO;
	GraphicsPSO GBufferSKinnedWirePSO;
	GraphicsPSO DeferredRenderingPSO;
	GraphicsPSO VolumeSmokePSO;

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;
};
