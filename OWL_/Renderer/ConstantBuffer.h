#pragma once

class ConstantBuffer
{
public:
	ConstantBuffer() = default;
	~ConstantBuffer() { Cleanup(); }

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, UINT CBSize, void* pInitData);

	void Upload();

	void Cleanup();

	inline UINT GetConstantBufferSize() { return m_CBSize; }

public:
	ID3D11Buffer* pBuffer = nullptr;
	void* pSystemMem = nullptr;

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;

	UINT m_CBSize = 0;
};