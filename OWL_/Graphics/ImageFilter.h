#pragma once

class ImageFilter
{
public:
	ImageFilter() = default;
	~ImageFilter() { Cleanup(); }

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ID3D11PixelShader* pPixelShader, int width, int height);

	void UpdateConstantBuffers(ID3D11DeviceContext* pContext);

	void Render(ID3D11DeviceContext* pContext) const;

	void Cleanup();

	void SetShaderResources(const std::vector<ID3D11ShaderResourceView*>& RESOURCES);
	void SetRenderTargets(const std::vector<ID3D11RenderTargetView*>& TARGETS);

public:
	ImageFilterConstData ConstantsData = { 0, };

protected:
	ID3D11PixelShader* m_pPixelShader = nullptr;
	ID3D11Buffer* m_pConstBuffer = nullptr;
	D3D11_VIEWPORT m_Viewport = { 0, };

	// Do not delete pointers.
	std::vector<ID3D11ShaderResourceView*> m_pSRVs;
	std::vector<ID3D11RenderTargetView*> m_pRTVs;
};
