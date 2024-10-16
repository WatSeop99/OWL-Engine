#pragma once

#include "../Graphics/ConstantDataType.h"
#include "../Graphics/ImageFilter.h"

class BaseRenderer;
class ConstantBuffer;
class Mesh;
class Texture;

class PostProcessor
{
public:
	struct PostProcessingBuffers
	{
		Texture* pBackBuffer = nullptr;
		Texture* pFloatBuffer = nullptr;
		Texture* pPrevBuffer = nullptr;
		Texture* pDepthBuffer = nullptr;
	};

public:
	PostProcessor() = default;
	~PostProcessor() { Cleanup(); }

	void Initialize(BaseRenderer* pRenderer, const PostProcessingBuffers& CONFIG, const int WIDTH, const int HEIGHT, const int BLOOMLEVELS);

	void Update();

	void Render();

	void Cleanup();

	inline ConstantBuffer* GetPostEffectConstantBuffer() { return m_pPostEffectsConstantBuffer; }

	void SetGlobalConstants(ConstantBuffer* const pGlobalConstants);

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
	Texture* m_pPostEffectsBuffer = nullptr;

	std::vector<ImageFilter> m_pBloomDownFilters;
	std::vector<ImageFilter> m_pBloomUpFilters;
	std::vector<ID3D11ShaderResourceView*> m_pBloomSRVs;
	std::vector<ID3D11RenderTargetView*> m_pBloomRTVs;

	// Do not delete these pointer.
	BaseRenderer* m_pRenderer = nullptr;
	ID3D11Buffer* m_pGlobalConstsGPU = nullptr;
	Texture* m_pBackBuffer = nullptr;
	Texture* m_pFloatBuffer = nullptr;
	Texture* m_pPrevBuffer = nullptr;
	Texture* m_pDepthBuffer = nullptr;
};