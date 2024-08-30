#include "../Common.h"
#include "../Graphics/GraphicsUtils.h"
#include "ConstantBuffer.h"

void ConstantBuffer::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, UINT CBSize, void* pInitData)
{
	_ASSERT(pDevice);
	_ASSERT(pContext);
	_ASSERT(CBSize > 0);

	m_pDevice = pDevice;
	m_pDevice->AddRef();
	m_pContext = pContext;
	m_pContext->AddRef();

	m_CBSize = CBSize;

	pSystemMem = Malloc(CBSize);
	if (!pSystemMem)
	{
		__debugbreak();
	}
	if (pInitData)
	{
		memcpy(pSystemMem, pInitData, CBSize);
	}
	else
	{
		ZeroMemory(pSystemMem, CBSize);
	}


	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = (CBSize + 255) & ~255;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA cosntantBufferData = {};
	cosntantBufferData.pSysMem = pSystemMem;

	hr = pDevice->CreateBuffer(&bufferDesc, &cosntantBufferData, &pBuffer);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pBuffer, "ConstantBuffer");
}

void ConstantBuffer::Upload()
{
	_ASSERT(m_pContext);
	_ASSERT(pBuffer);
	_ASSERT(pSystemMem);

	m_pContext->UpdateSubresource(pBuffer, 0, nullptr, pSystemMem, 0, 0);
}

void ConstantBuffer::Cleanup()
{
	if (pSystemMem)
	{
		delete pSystemMem;
		pSystemMem = nullptr;
	}
	m_CBSize = 0;
	SAFE_RELEASE(pBuffer);
	SAFE_RELEASE(m_pDevice);
	SAFE_RELEASE(m_pContext);
}
