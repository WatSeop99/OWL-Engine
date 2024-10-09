#pragma once

#include "../Graphics/ConstantDataType.h"
#include "../Graphics/ImageFilter.h"

class BaseRenderer;
class ConstantBuffer;
class Mesh;

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
	PostProcessor() = default;
	~PostProcessor() { Cleanup(); }

	void Initialize(BaseRenderer* pRenderer, const PostProcessingBuffers& CONFIG, const int WIDTH, const int HEIGHT, const int BLOOMLEVELS);

	void Update();

	void Render();

	void Cleanup();

	inline ConstantBuffer* GetPostEffectConstantBuffer() { return m_pPostEffectsConstantBuffer; }

protected:
	void createPostBackBuffers();
	void createImageResources(int width, int height, ID3D11ShaderResourceView** ppSrv, ID3D11RenderTargetView** ppRtv);

	void renderPostEffects();
	void renderPostProcessing();
	void renderImageFilter(const ImageFilter& IMAGE_FILTER);

	void setViewport();
	void setRenderConfig(const PostProcessingBuffers& CONFIG);
	void setGlobalConsts(ID3D11Buffer** ppGlobalConstsGPU);

public:
	int PostEffectsUpdateFlag = 0;

	ImageFilter CombineFilter;
	int CombineUpdateFlag = 0;

private:
	D3D11_VIEWPORT m_Viewport = { 0, };
	UINT m_ScreenWidth = 0;
	UINT m_ScreenHeight = 0;

	ConstantBuffer* m_pPostEffectsConstantBuffer = nullptr;

	ID3D11Texture2D* m_pPostEffectsBuffer = nullptr;
	ID3D11RenderTargetView* m_pPostEffectsRTV = nullptr;
	ID3D11ShaderResourceView* m_pPostEffectsSRV = nullptr;

	std::vector<ImageFilter> m_pBloomDownFilters;
	std::vector<ImageFilter> m_pBloomUpFilters;
	std::vector<ID3D11ShaderResourceView*> m_pBloomSRVs;
	std::vector<ID3D11RenderTargetView*> m_pBloomRTVs;

	// Do not delete these pointer.
	BaseRenderer* m_pRenderer = nullptr;
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