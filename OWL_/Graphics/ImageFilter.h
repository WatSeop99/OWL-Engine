#pragma once

class ConstantBuffer;

class ImageFilter
{
public:
	ImageFilter() = default;
	~ImageFilter() { Cleanup(); }

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ID3D11PixelShader* pPixelShader, int width, int height);

	void UpdateConstantBuffers(ID3D11DeviceContext* pContext);

	void Render(ID3D11DeviceContext* pContext) const;

	void Cleanup();

	inline ConstantBuffer* GetConstantBufferPtr() { return m_pConstantBuffer; }

	void SetShaderResources(const std::vector<ID3D11ShaderResourceView*>& RESOURCES);
	void SetRenderTargets(const std::vector<ID3D11RenderTargetView*>& TARGETS);

protected:
	D3D11_VIEWPORT m_Viewport = { 0, };
	ConstantBuffer* m_pConstantBuffer = nullptr;

	// Do not delete pointers.
	std::vector<ID3D11ShaderResourceView*> m_pSRVs;
	std::vector<ID3D11RenderTargetView*> m_pRTVs;
	ID3D11PixelShader* m_pPixelShader = nullptr;
};
