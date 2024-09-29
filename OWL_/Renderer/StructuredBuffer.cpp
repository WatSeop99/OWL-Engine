#include "../Common.h"
#include "../Graphics/GraphicsUtils.h"
#include "StructuredBuffer.h"

void StructuredBuffer::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, UINT sizePerElem, UINT elemCount, void* pInitData)
{
	_ASSERT(pDevice);
	_ASSERT(pContext);
	_ASSERT(sizePerElem > 0);
	_ASSERT(elemCount > 0);

	m_pDevice = pDevice;
	m_pDevice->AddRef();
	m_pContext = pContext;
	m_pContext->AddRef();

	m_SizePerElem = sizePerElem;
	m_ElemCount = elemCount;


	pSystemMem = malloc(sizePerElem * elemCount);
	if (!pSystemMem)
	{
		__debugbreak();
	}
	if (pInitData)
	{
		memcpy(pSystemMem, pInitData, sizePerElem * elemCount);
	}
	else
	{
		ZeroMemory(pSystemMem, sizePerElem * elemCount);
	}


	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC structuredBufferDesc = {};
	structuredBufferDesc.ByteWidth = sizePerElem * elemCount;
	structuredBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	structuredBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	structuredBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	structuredBufferDesc.StructureByteStride = sizePerElem;

	hr = m_pDevice->CreateBuffer(&structuredBufferDesc, nullptr, &pBuffer);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pBuffer, "StructuredBuffer");


	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.BufferEx.NumElements = elemCount;

	hr = m_pDevice->CreateShaderResourceView(pBuffer, &srvDesc, &pSRV);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pSRV, "StructuredBuffer::SRV");


	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = elemCount;

	hr = m_pDevice->CreateUnorderedAccessView(pBuffer, &uavDesc, &pUAV);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(pUAV, "StructuredBuffe::UAV");


	D3D11_BUFFER_DESC stagingBufferDesc = {};
	stagingBufferDesc.ByteWidth = sizePerElem * elemCount;
	stagingBufferDesc.Usage = D3D11_USAGE_STAGING;
	stagingBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
	stagingBufferDesc.StructureByteStride = sizePerElem;

	hr = m_pDevice->CreateBuffer(&stagingBufferDesc, nullptr, &m_pStagingBuffer);
	BREAK_IF_FAILED(hr);
	SET_DEBUG_INFO_TO_OBJECT(m_pStagingBuffer, "StructuredBuffer::StagingBuffer");
}

void StructuredBuffer::Upload()
{
	_ASSERT(m_pContext);
	_ASSERT(m_pStagingBuffer);
	_ASSERT(pBuffer);
	_ASSERT(pSystemMem);

	D3D11_MAPPED_SUBRESOURCE mappedResource = {};
	m_pContext->Map(m_pStagingBuffer, 0, D3D11_MAP_WRITE, 0, &mappedResource);
	memcpy(mappedResource.pData, pSystemMem, m_SizePerElem * m_ElemCount);
	m_pContext->Unmap(m_pStagingBuffer, 0);

	m_pContext->CopyResource(pBuffer, m_pStagingBuffer);
}

void StructuredBuffer::Download()
{
	_ASSERT(m_pContext);
	_ASSERT(m_pStagingBuffer);
	_ASSERT(pBuffer);
	_ASSERT(pSystemMem);

	m_pContext->CopyResource(m_pStagingBuffer, pBuffer);

	D3D11_MAPPED_SUBRESOURCE mappedResource = {};
	m_pContext->Map(pBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);
	memcpy(pSystemMem, mappedResource.pData, m_SizePerElem * m_ElemCount);
	m_pContext->Unmap(pBuffer, 0);
}

void StructuredBuffer::Cleanup()
{
	if (pSystemMem)
	{
		delete pSystemMem;
		pSystemMem = nullptr;
	}
	m_SizePerElem = 0;
	m_ElemCount = 0;

	SAFE_RELEASE(pSRV);
	SAFE_RELEASE(pUAV);
	SAFE_RELEASE(pBuffer);
	SAFE_RELEASE(m_pStagingBuffer);
	SAFE_RELEASE(m_pDevice);
	SAFE_RELEASE(m_pContext);
}
