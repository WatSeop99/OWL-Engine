#pragma once

HRESULT ReadEXRImage(const WCHAR* pszFileName, std::vector<UINT8>& image, int* pWidth, int* pHeight, DXGI_FORMAT* pPixelFormat);
HRESULT ReadImage(const WCHAR* pszFileName, std::vector<UINT8>& image, int* pWidth, int* pHeight, DXGI_FORMAT* pPixelFormat, const bool bUSE_SRGB = false);
HRESULT ReadImage(const WCHAR* pszAlbedoFileName, const WCHAR* pszOpacityFileName, std::vector<UINT8>& image, int* pWidth, int* pHeight);

HRESULT CreateVertexShaderAndInputLayout(ID3D11Device* pDevice, const wchar_t* pszFileName, const D3D11_INPUT_ELEMENT_DESC* pINPUT_ELEMENTS, const UINT ELEMENTS_SIZE, const D3D_SHADER_MACRO* pSHADER_MACROS, ID3D11VertexShader** ppVertexShader, ID3D11InputLayout** ppInputLayout);
HRESULT CreateHullShader(ID3D11Device* pDevice, const wchar_t* pszFileName, ID3D11HullShader** ppHullShader);
HRESULT CreateDomainShader(ID3D11Device* pDevice, const wchar_t* pszFileName, ID3D11DomainShader** ppDomainShader);
HRESULT CreateGeometryShader(ID3D11Device* pDevice, const wchar_t* pszFileName, ID3D11GeometryShader** ppGeometryShader);
HRESULT CreateComputeShader(ID3D11Device* pDevice, const wchar_t* pszFileName, ID3D11ComputeShader** ppComputeShader);
HRESULT CreatePixelShader(ID3D11Device* pDevice, const wchar_t* pszFileName, ID3D11PixelShader** ppPixelShader);
HRESULT CreateIndexBuffer(ID3D11Device* pDevice, const std::vector<uint32_t>& INDICES, ID3D11Buffer** pppIndexBuffer);

template <typename VERTEX>
HRESULT CreateVertexBuffer(ID3D11Device* pDevice, const std::vector<VERTEX>& VERTICES, ID3D11Buffer** ppVertexBuffer)
{
	_ASSERT(pDevice);
	_ASSERT(!(*ppVertexBuffer));

	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = (UINT)(sizeof(VERTEX) * VERTICES.size());
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0; // 0은 CPU 접근이 필요없음을 나타냄.
	bufferDesc.StructureByteStride = sizeof(VERTEX);

	D3D11_SUBRESOURCE_DATA vertexBufferData = {};
	vertexBufferData.pSysMem = VERTICES.data();

	hr = pDevice->CreateBuffer(&bufferDesc, &vertexBufferData, ppVertexBuffer);
	return hr;
}

template <typename INSTANCE>
HRESULT CreateInstanceBuffer(ID3D11Device* pDevice, const std::vector<INSTANCE>& INSTANCES, ID3D11Buffer** ppInstanceBuffer)
{
	_ASSERT(pDevice);
	_ASSERT(ppInstanceBuffer);
	_ASSERT(!(*ppInstanceBuffer));

	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = (UINT)(sizeof(INSTANCE) * INSTANCES.size());
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.StructureByteStride = sizeof(INSTANCE);

	D3D11_SUBRESOURCE_DATA instanceBufferData = {};
	instanceBufferData.pSysMem = INSTANCES.data();

	hr = pDevice->CreateBuffer(&bufferDesc, &instanceBufferData, ppInstanceBuffer);
	return hr;
}

template <typename CONSTANT>
HRESULT CreateConstBuffer(ID3D11Device* pDevice, const CONSTANT& CONSTANT_BUFFER_DATA, ID3D11Buffer** ppConstantBuffer)
{
	_ASSERT((sizeof(CONSTANT) % 16) == 0); // Constant Buffer size must be 16-byte aligned.
	_ASSERT(pDevice);
	_ASSERT(ppConstantBuffer);
	_ASSERT(!(*ppConstantBuffer));

	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC bufferDesc = {};
	// 256byte 정렬. dx12에서는 이렇게 설정함. 
	// 구조체에서 alignas(256)를 사용할 경우, debug new가 _align_malloc_dbg()를 호출하지 않는 문제가 발생. 
	// 이를 방지하기 위해 상수버퍼 생성 시, 256byte로 정렬하여 생성.
	bufferDesc.ByteWidth = (sizeof(CONSTANT_BUFFER_DATA) + 255) & ~255;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA cosntantBufferData = {};
	cosntantBufferData.pSysMem = &CONSTANT_BUFFER_DATA;

	hr = pDevice->CreateBuffer(&bufferDesc, &cosntantBufferData, ppConstantBuffer);
	return hr;
}

template <typename DATA>
void UpdateBuffer(ID3D11DeviceContext* pContext, const std::vector<DATA>& BUFFER_DATA, ID3D11Buffer* pBuffer)
{
	_ASSERT(pContext);
	_ASSERT(pBuffer);

	pContext->UpdateSubresource(pBuffer, 0, nullptr, BUFFER_DATA.data(), 0, 0);
}

template <typename DATA>
void UpdateBuffer(ID3D11DeviceContext* pContext, const DATA& BUFFER_DATA, ID3D11Buffer* pBuffer)
{
	_ASSERT(pContext);
	_ASSERT(pBuffer);

	pContext->UpdateSubresource(pBuffer, 0, nullptr, &BUFFER_DATA, 0, 0);
}

//HRESULT CreateTexture(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const wchar_t* pszFileName, const bool bUSE_SRGB, ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppTextureResourceView);
//HRESULT CreateTexture(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const wchar_t* pszAlbedoFileName, const wchar_t* pszOpacityFileName, const bool bUSE_SRGB, ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppTextureResourceView);
//HRESULT CreateTexture2D(ID3D11Device* pDevice, const int WIDTH, const int HEIGHT, const DXGI_FORMAT PIXEL_FORMAT, const bool bIS_DEPTH_STENCIL,
//						ID3D11Texture2D** ppTexture, ID3D11RenderTargetView** ppRTV, ID3D11ShaderResourceView** ppSRV, ID3D11DepthStencilView** ppDSV, ID3D11UnorderedAccessView** ppUAV);
//HRESULT CreateTexture2D(ID3D11Device* pDevice, D3D11_TEXTURE2D_DESC& desc, ID3D11Texture2D** ppTexture, ID3D11RenderTargetView** ppRTV, ID3D11ShaderResourceView** ppSRV, ID3D11DepthStencilView** ppDSV, ID3D11UnorderedAccessView** ppUAV);
//HRESULT CreateTexture3D(ID3D11Device* pDevice, const int WIDTH, const int HEIGHT, const int DEPTH, const DXGI_FORMAT PIXEL_FORMAT, const bool bIS_DEPTH_STENCIL, const std::vector<float>& INIT_DATA,
//						ID3D11Texture3D** ppTexture, ID3D11RenderTargetView** ppRTV, ID3D11ShaderResourceView** ppSRV, ID3D11DepthStencilView** ppDSV, ID3D11UnorderedAccessView** ppUAV);
//HRESULT CreateTexture3D(ID3D11Device* pDevice, D3D11_TEXTURE3D_DESC& desc, const std::vector<float>& INIT_DATA,
//						ID3D11Texture3D** ppTexture, ID3D11RenderTargetView** ppRTV, ID3D11ShaderResourceView** ppSRV, ID3D11DepthStencilView** ppDSV, ID3D11UnorderedAccessView** ppUAV);
//
//HRESULT CreateStagingBuffer(ID3D11Device* pDevice, const UINT NUM_ELEMENTS, const UINT SIZE_ELEMENT, const void* pINIT_DATA, ID3D11Buffer** ppBuffer);
//void CopyFromStagingBuffer(ID3D11DeviceContext* pContext, ID3D11Buffer* pBuffer, UINT size, void* pDest);
//void CopyToStagingBuffer(ID3D11DeviceContext* pContext, ID3D11Buffer* pBuffer, UINT size, void* pSrc);
//
//HRESULT CreateStructuredBuffer(ID3D11Device* pDevice, const UINT NUM_ELEMENTS, const UINT SIZE_ELEMENT, const void* pINIT_DATA, ID3D11Buffer** ppBuffer, ID3D11ShaderResourceView** ppSRV, ID3D11UnorderedAccessView** ppUAV);
//
//HRESULT CreateIndirectArgsBuffer(ID3D11Device* pDevice, const UINT NUM_ELEMENTS, const UINT SIZE_ELEMENT, const void* pINIT_DATA, ID3D11Buffer** ppBuffer);
//
//HRESULT CreateAppendBuffer(ID3D11Device* pDevice, const UINT NUM_ELEMENTS, const UINT SIZE_ELEMENT, const void* pINIT_DATA, ID3D11Buffer** ppBuffer, ID3D11ShaderResourceView** ppSRV, ID3D11UnorderedAccessView** ppUAV);
//
//HRESULT CreateMetallicRoughnessTexture(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const wchar_t* pszMetallicFileName, const wchar_t* pszRoughnessFileName, ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppSRV);
//HRESULT CreateTextureArray(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<std::wstring>& FILE_NAMES, ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppTextureResourceView);
//HRESULT CreateDDSTexture(ID3D11Device* pDevice, const wchar_t* pszFileName, const bool bIsCubeMap, ID3D11ShaderResourceView** ppTextureResourceView);
//
//ID3D11Texture2D* CreateStagingTexture(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const int WIDTH, const int HEIGHT, const std::vector<uint8_t>& IMAGE, const DXGI_FORMAT PIXEL_FORMAT, const int MIP_LEVELS = 1, const int ARRAY_SIZE = 1);
//ID3D11Texture3D* CreateStagingTexture3D(ID3D11Device* pDevice, const int WIDTH, const int HEIGHT, const int DEPTH, const DXGI_FORMAT PIXEL_FORMAT);
//
//void WriteToPngFile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ID3D11Texture2D* pTextureToWrite, const wchar_t* pszFileName);

UINT64 GetPixelSize(DXGI_FORMAT pixelFormat);
