#pragma once

#include "../Graphics/GraphicsUtils.h"

class StructuredBuffer
{
public:
	StructuredBuffer() = default;
	~StructuredBuffer() { Cleanup(); }

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, UINT sizePerElem, UINT elemCount, void* pInitData);

	void Upload();

	void Download();

	void Cleanup();

	inline UINT GetSizePerElem() { return m_SizePerElem; }
	inline UINT GetElemCount() { return m_ElemCount; }

public:
	ID3D11Buffer* pBuffer = nullptr;
	ID3D11ShaderResourceView* pSRV = nullptr;
	ID3D11UnorderedAccessView* pUAV = nullptr;
	void* pSystemMem = nullptr;

private:
	ID3D11Buffer* m_pStagingBuffer = nullptr;
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;

	UINT m_SizePerElem = 0;
	UINT m_ElemCount = 0;
};
