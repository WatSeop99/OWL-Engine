#pragma once

#include "ImageFilter.h"
#include "../Geometry/Mesh.h"

namespace Core
{
	class PostProcessor
	{
	public:
		struct PostProcessingBuffers
		{
			ID3D11Buffer* pGlobalConstsGPU = nullptr;
			ID3D11Texture2D* pBackBuffer = nullptr;
			ID3D11Texture2D* pFloatBuffer = nullptr;
			ID3D11Texture2D* pResolvedBuffer = nullptr;
			ID3D11Texture2D* pPrevBuffer = nullptr;
			ID3D11RenderTargetView* pBackBufferRTV = nullptr;
			ID3D11ShaderResourceView* pResolvedSRV = nullptr;
			ID3D11ShaderResourceView* pPrevSRV = nullptr;
			ID3D11ShaderResourceView* pDepthOnlySRV = nullptr;
		};

	public:
		PostProcessor() :
			m_Viewport{ 0, }, 
			m_ScreenWidth(0), 
			m_ScreenHeight(0),
			PostEffectsUpdateFlag(0),
			CombineUpdateFlag(0)
		{ }
		~PostProcessor() { destroy(); }

		void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const struct PostProcessingBuffers& CONFIG, const int WIDTH, const int HEIGHT, const int BLOOMLEVELS);

		void Update(ID3D11DeviceContext* pContext);

		void Render(ID3D11DeviceContext* pContext);

	protected:
		void createPostBackBuffers(ID3D11Device* pDevice);
		void createImageResources(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, int width, int height, ID3D11ShaderResourceView** ppSrv, ID3D11RenderTargetView** ppRtv);
		
		void renderPostEffects(ID3D11DeviceContext* pContext);
		void renderPostProcessing(ID3D11DeviceContext* pContext);
		void renderImageFilter(ID3D11DeviceContext* pContext, const ImageFilter& IMAGE_FILTER);

		void setViewport(ID3D11DeviceContext* pContext);
		void setRenderConfig(const struct PostProcessingBuffers& CONFIG);
		void setPipelineState(ID3D11DeviceContext* pContext, Graphics::GraphicsPSO& PSO);
		void setGlobalConsts(ID3D11DeviceContext* pContext, ID3D11Buffer** ppGlobalConstsGPU);

		void destroy();

	public:
		PostEffectsConstants PostEffectsConstsCPU;
		int PostEffectsUpdateFlag;

		ImageFilter CombineFilter;
		int CombineUpdateFlag;

		struct Geometry::Mesh* pMesh = nullptr;

	private:
		D3D11_VIEWPORT m_Viewport;
		int m_ScreenWidth;
		int m_ScreenHeight;

		ID3D11Buffer* m_pPostEffectsConstsGPU = nullptr;

		ID3D11Texture2D* m_pPostEffectsBuffer = nullptr;
		ID3D11RenderTargetView* m_pPostEffectsRTV = nullptr;
		ID3D11ShaderResourceView* m_pPostEffectsSRV = nullptr;

		std::vector<ImageFilter> m_pBloomDownFilters;
		std::vector<ImageFilter> m_pBloomUpFilters;
		std::vector<ID3D11ShaderResourceView*> m_pBloomSRVs;
		std::vector<ID3D11RenderTargetView*> m_pBloomRTVs;

		// Do not delete these pointer.
		ID3D11Buffer* m_pGlobalConstsGPU = nullptr;
		ID3D11Texture2D* m_pBackBuffer = nullptr;
		ID3D11Texture2D* m_pFloatBuffer = nullptr;
		ID3D11Texture2D* m_pResolvedBuffer = nullptr;
		ID3D11Texture2D* m_pPrevBuffer = nullptr; // 간단한 모션 블러 효과
		ID3D11RenderTargetView* m_pBackBufferRTV = nullptr;
		ID3D11ShaderResourceView* m_pResolvedSRV = nullptr;
		ID3D11ShaderResourceView* m_pPrevSRV = nullptr;
		ID3D11ShaderResourceView* m_pDepthOnlySRV = nullptr;
	};
}