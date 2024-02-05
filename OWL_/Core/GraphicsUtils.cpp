// #define _CRT_SECURE_NO_WARNINGS // stb_image_write compile error fix

#include <DirectXTex.h>
#include <DirectXTexEXR.h>
#include <directxtk/DDSTextureLoader.h>
#include <float.h>
#include "../Common.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#include "GraphicsUtils.h"

HRESULT ReadEXRImage(const wchar_t* pszFileName, std::vector<uint8_t>& image, int* pWidth, int* pHeight, DXGI_FORMAT* pPixelFormat)
{
	HRESULT hr = S_OK;

	DirectX::TexMetadata metaData;
	DirectX::ScratchImage scratchImage;

	hr = GetMetadataFromEXRFile(pszFileName, metaData);
	if (FAILED(hr))
	{
		goto LB_RET;
	}

	hr = LoadFromEXRFile(pszFileName, nullptr, scratchImage);
	if (FAILED(hr))
	{
		goto LB_RET;
	}

	*pWidth = (int)(metaData.width);
	*pHeight = (int)(metaData.height);
	*pPixelFormat = metaData.format;

	// Debug information
	/*std::wstring debugStr(pszFileName);
	debugStr += L" " + std::to_wstring(metaData.width) + L" " + std::to_wstring(metaData.height) + std::to_wstring(metaData.format);
	OutputDebugStringW(debugStr.c_str());*/

	image.resize(scratchImage.GetPixelsSize());
	memcpy(image.data(), scratchImage.GetPixels(), image.size());

	// Debug. 데이터 범위 확인.
	/*
	std::vector<float> f32;
	uint16_t* pF16 = nullptr;
	float minValue = FLT_MAX;
	float maxValue = FLT_MIN;
	f32.resize(image.size() / 2);
	pF16 = (uint16_t*)image.data();
	for (int i = 0, size = image.size() / 2; i < size; ++i)
	{
		f32[i] = fp16_ieee_to_fp32_value(f16[i]);
		minValue = (minValue < f32[i] ? minValue : f32[i]);
		maxValue = (maxValue > f32[i] ? maxValue : f32[i]);
	}*/

LB_RET:
	return hr;
}

HRESULT ReadImage(const wchar_t* pszFileName, std::vector<uint8_t>& image, int* pWidth, int* pHeight)
{
	HRESULT hr = S_OK;
	int channels = 0;

	char pFileName[MAX_PATH];
	if (!WideCharToMultiByte(CP_ACP, 0, pszFileName, -1, pFileName, MAX_PATH, nullptr, nullptr))
	{
		pFileName[0] = '\0';
	}

	unsigned char* pImg = stbi_load(pFileName, pWidth, pHeight, &channels, 0);

	// 4채널로 만들어 복사.
	image.resize((*pWidth) * (*pHeight) * 4);

	if (channels == 1)
	{
		for (int i = 0, size = (*pWidth) * (*pHeight); i < size; ++i)
		{
			uint8_t g = pImg[i * channels];
			for (int c = 0; c < 4; ++c)
			{
				image[4 * i + c] = g;
			}
		}
	}
	else if (channels == 2)
	{
		for (int i = 0, size = (*pWidth) * (*pHeight); i < size; ++i)
		{
			for (int c = 0; c < 2; ++c)
			{
				image[4 * i + c] = pImg[i * channels + c];
			}
			image[4 * i + 2] = 255;
			image[4 * i + 3] = 255;
		}
	}
	else if (channels == 3)
	{
		for (int i = 0, size = (*pWidth) * (*pHeight); i < size; ++i)
		{
			for (int c = 0; c < 3; ++c)
			{
				image[4 * i + c] = pImg[i * channels + c];
			}
			image[4 * i + 3] = 255;
		}
	}
	else if (channels == 4)
	{
		for (int i = 0, size = (*pWidth) * (*pHeight); i < size; ++i)
		{
			for (int c = 0; c < 4; ++c)
			{
				image[4 * i + c] = pImg[i * channels + c];
			}
		}
	}
	else
	{
		hr = E_FAIL;
	}

	free(pImg);

	return hr;
}

HRESULT ReadImage(const wchar_t* pszAlbedoFileName, const wchar_t* pszOpacityFileName, std::vector<uint8_t>& image, int* pWidth, int* pHeight)
{
	std::vector<uint8_t> opacityImage;
	HRESULT hr = ReadImage(pszAlbedoFileName, image, pWidth, pHeight);
	if (FAILED(hr))
	{
		goto LB_RET;
	}

	{
		int opaWidth = 0;
		int opaHeight = 0;

		hr = ReadImage(pszOpacityFileName, opacityImage, &opaWidth, &opaHeight);
		if (FAILED(hr))
		{
			goto LB_RET;
		}

		_ASSERT(*pWidth == opaWidth && *pHeight == opaHeight);
	}

	for (int j = 0; j < *pHeight; ++j)
	{
		for (int i = 0; i < *pWidth; ++i)
		{
			image[3 + 4 * i + 4 * (*pWidth) * j] = opacityImage[4 * i + 4 * (*pWidth) * j]; // 알파채널 복사.
		}
	}

LB_RET:
	return hr;
}

HRESULT CreateTextureHelper(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const int WIDTH, const int HEIGHT, const std::vector<uint8_t>& image, const DXGI_FORMAT PIXEL_FORMAT,
							ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppSRV)
{
	_ASSERT(pDevice != nullptr);
	_ASSERT(pContext != nullptr);
	_ASSERT((*ppTexture) == nullptr);
	_ASSERT((*ppSRV) == nullptr);

	HRESULT hr = S_OK;

	// 스테이징 텍스쳐를 만들고 CPU에서 이미지 복사.
	ID3D11Texture2D* pStagingTexture = Graphics::CreateStagingTexture(pDevice,
																	  pContext,
																	  WIDTH, HEIGHT,
																	  image,
																	  PIXEL_FORMAT);
	if (pStagingTexture == nullptr)
	{
		hr = E_FAIL;
		goto LB_RET;
	}
	SET_DEBUG_INFO_TO_OBJECT(pStagingTexture, "GraphicsUtils::CreateTextureHelper::pStagingTexture");

	{
		D3D11_TEXTURE2D_DESC textureDesc = { 0, };
		textureDesc.Width = WIDTH;
		textureDesc.Height = HEIGHT;
		textureDesc.MipLevels = 0;
		textureDesc.ArraySize = 1;
		textureDesc.Format = PIXEL_FORMAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT; // 스테이징 텍스쳐로부터 복사 가능.
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS; // 밉맵 사용.

		// 초기 데이터 없이 텍스쳐 생성(검정색).
		hr = pDevice->CreateTexture2D(&textureDesc, nullptr, ppTexture);
		if (FAILED(hr))
		{
			RELEASE(pStagingTexture);
			goto LB_RET;
		}

		// 스테이징 텍스쳐로부터 가장 해상도가 높은 이미지 복사.
		pContext->CopySubresourceRegion(*ppTexture,
										0,
										0, 0, 0,
										pStagingTexture,
										0,
										nullptr);
		RELEASE(pStagingTexture);

		// ResourceView 만들기.
		hr = pDevice->CreateShaderResourceView(*ppTexture, 0, ppSRV);
		if (FAILED(hr))
		{
			goto LB_RET;
		}

		// 해상도를 낮춰가며 밉맵 생성.
		pContext->GenerateMips(*ppSRV);
	}

LB_RET:
	return hr;
}

namespace Graphics
{
	using namespace std;
	using namespace DirectX;

	//void CheckResult(HRESULT hr, ID3DBlob* errorBlob)
	//{
	//	if (FAILED(hr))
	//	{
	//		// 파일이 없을 경우.
	//		if ((hr & D3D11_ERROR_FILE_NOT_FOUND) != 0)
	//		{
	//			OutputDebugStringA("File not found.\n");
	//		}

	//		// 에러 메시지가 있으면 출력.
	//		if (errorBlob)
	//		{
	//			OutputDebugStringA("Shader compile error\n");
	//			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	//			OutputDebugStringA("\n");
	//		}
	//	}
	//}

	HRESULT CreateVertexShaderAndInputLayout(ID3D11Device* pDevice, const wchar_t* pszFileName, const D3D11_INPUT_ELEMENT_DESC* pINPUT_ELEMENTS, const UINT ELEMENTS_SIZE, const D3D_SHADER_MACRO* pSHADER_MACROS,
											 ID3D11VertexShader** ppVertexShader, ID3D11InputLayout** ppInputLayout)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT((*ppVertexShader) == nullptr);

		HRESULT hr = S_OK;

		ID3DBlob* pShaderBlob = nullptr;
		ID3DBlob* pErrorBlob = nullptr;
		UINT compileFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
		// compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_OPTIMIZATION_LEVEL0;
#endif

		hr = D3DCompileFromFile(pszFileName, pSHADER_MACROS, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", compileFlags, 0,
								&pShaderBlob, &pErrorBlob);
		if (FAILED(hr))
		{
			if (pErrorBlob)
			{
				OutputDebugStringA((const char*)(pErrorBlob->GetBufferPointer()));
				RELEASE(pErrorBlob);
			}
			goto LB_RET;
		}

		hr = pDevice->CreateVertexShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, ppVertexShader);
		if (FAILED(hr))
		{
			RELEASE(pShaderBlob);
			goto LB_RET;
		}

		if ((*ppInputLayout) == nullptr)
		{
			hr = pDevice->CreateInputLayout(pINPUT_ELEMENTS, ELEMENTS_SIZE, pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), ppInputLayout);
		}
		RELEASE(pShaderBlob);

	LB_RET:
		return hr;
	}

	HRESULT CreateHullShader(ID3D11Device* pDevice, const wchar_t* pszFileName, ID3D11HullShader** ppHullShader)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT((*ppHullShader) == nullptr);

		HRESULT hr = S_OK;

		ID3DBlob* pShaderBlob = nullptr;
		ID3DBlob* pErrorBlob = nullptr;
		UINT compileFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
		// compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_OPTIMIZATION_LEVEL0;
#endif

		hr = D3DCompileFromFile(pszFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "hs_5_0", compileFlags, 0,
								&pShaderBlob, &pErrorBlob);
		if (FAILED(hr))
		{
			if (pErrorBlob != nullptr)
			{
				OutputDebugStringA((const char*)(pErrorBlob->GetBufferPointer()));
				RELEASE(pErrorBlob);
			}
			goto LB_RET;
		}

		hr = pDevice->CreateHullShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, ppHullShader);
		RELEASE(pShaderBlob);

	LB_RET:
		return hr;
	}

	HRESULT CreateDomainShader(ID3D11Device* pDevice, const wchar_t* pszFileName, ID3D11DomainShader** ppDomainShader)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT((*ppDomainShader) == nullptr);

		HRESULT hr = S_OK;

		ID3DBlob* pShaderBlob = nullptr;
		ID3DBlob* pErrorBlob = nullptr;
		UINT compileFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
		// compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_OPTIMIZATION_LEVEL0;
#endif

		hr = D3DCompileFromFile(pszFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ds_5_0", compileFlags, 0,
								&pShaderBlob, &pErrorBlob);
		if (FAILED(hr))
		{
			if (pErrorBlob != nullptr)
			{
				OutputDebugStringA((const char*)(pErrorBlob->GetBufferPointer()));
				RELEASE(pErrorBlob);
			}
			goto LB_RET;
		}

		hr = pDevice->CreateDomainShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, ppDomainShader);
		RELEASE(pShaderBlob);

	LB_RET:
		return hr;
	}

	HRESULT CreatePixelShader(ID3D11Device* pDevice, const wchar_t* pszFileName, ID3D11PixelShader** ppPixelShader)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT((*ppPixelShader) == nullptr);

		HRESULT hr = S_OK;

		ID3DBlob* pShaderBlob = nullptr;
		ID3DBlob* pErrorBlob = nullptr;
		UINT compileFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
		// compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_OPTIMIZATION_LEVEL0;
#endif

		hr = D3DCompileFromFile(pszFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", compileFlags, 0,
								&pShaderBlob, &pErrorBlob);
		if (FAILED(hr))
		{
			if (pErrorBlob != nullptr)
			{
				OutputDebugStringA((const char*)(pErrorBlob->GetBufferPointer()));
				RELEASE(pErrorBlob);
			}
			goto LB_RET;
		}

		hr = pDevice->CreatePixelShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, ppPixelShader);
		RELEASE(pShaderBlob);

	LB_RET:
		return hr;
	}

	HRESULT CreateIndexBuffer(ID3D11Device* pDevice, const vector<uint32_t>& INDICES, ID3D11Buffer** ppIndexBuffer)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT((*ppIndexBuffer) == nullptr);

		HRESULT hr = S_OK;

		D3D11_BUFFER_DESC bufferDesc = { 0, };
		bufferDesc.ByteWidth = (UINT)(sizeof(uint32_t) * INDICES.size());
		bufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // 초기화 후 변경 x.
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.StructureByteStride = sizeof(uint32_t);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0, };
		indexBufferData.pSysMem = INDICES.data();

		hr = pDevice->CreateBuffer(&bufferDesc, &indexBufferData, ppIndexBuffer);
		return hr;
	}

	HRESULT CreateGeometryShader(ID3D11Device* pDevice, const wchar_t* pszFileName, ID3D11GeometryShader** ppGeometryShader)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT((*ppGeometryShader) == nullptr);

		HRESULT hr = S_OK;

		ID3DBlob* pShaderBlob = nullptr;
		ID3DBlob* pErrorBlob = nullptr;
		UINT compileFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
		// compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_OPTIMIZATION_LEVEL0;
#endif

		hr = D3DCompileFromFile(pszFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "gs_5_0", compileFlags, 0,
								&pShaderBlob, &pErrorBlob);
		if (FAILED(hr))
		{
			if (pErrorBlob != nullptr)
			{
				OutputDebugStringA((const char*)(pErrorBlob->GetBufferPointer()));
				RELEASE(pErrorBlob);
			}
			goto LB_RET;
		}

		hr = pDevice->CreateGeometryShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, ppGeometryShader);
		RELEASE(pShaderBlob);

	LB_RET:
		return hr;
	}

	HRESULT CreateComputeShader(ID3D11Device* pDevice, const wchar_t* pszFileName, ID3D11ComputeShader** ppComputeShader)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT((*ppComputeShader) == nullptr);

		HRESULT hr = S_OK;

		ID3DBlob* pShaderBlob = nullptr;
		ID3DBlob* pErrorBlob = nullptr;
		UINT compileFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
		// compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_OPTIMIZATION_LEVEL0;
#endif

		hr = D3DCompileFromFile(pszFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "cs_5_0", compileFlags, 0,
								&pShaderBlob, &pErrorBlob);
		if (FAILED(hr))
		{
			if (pErrorBlob != nullptr)
			{
				OutputDebugStringA((const char*)(pErrorBlob->GetBufferPointer()));
				RELEASE(pErrorBlob);
			}
			goto LB_RET;
		}

		hr = pDevice->CreateComputeShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, ppComputeShader);
		RELEASE(pShaderBlob);

	LB_RET:
		return hr;
	}

	ID3D11Texture2D* CreateStagingTexture(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const int WIDTH, const int HEIGHT, const std::vector<uint8_t>& IMAGE, const DXGI_FORMAT PIXEL_FORMAT, const int MIP_LEVELS, const int ARRAY_SIZE)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT(pContext != nullptr);

		HRESULT hr = S_OK;

		ID3D11Texture2D* pStagingTexture = nullptr;
		D3D11_TEXTURE2D_DESC textureDesc = { 0, };
		textureDesc.Width = WIDTH;
		textureDesc.Height = HEIGHT;
		textureDesc.MipLevels = MIP_LEVELS;
		textureDesc.ArraySize = ARRAY_SIZE;
		textureDesc.Format = PIXEL_FORMAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_STAGING;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;

		hr = pDevice->CreateTexture2D(&textureDesc, nullptr, &pStagingTexture);
		if (FAILED(hr))
		{
			goto LB_RET;
		}

		{
			size_t pixelSize = GetPixelSize(PIXEL_FORMAT);
			D3D11_MAPPED_SUBRESOURCE mappedResource = { 0, };

			pContext->Map(pStagingTexture, 0, D3D11_MAP_WRITE, 0, &mappedResource);
			uint8_t* pData = (uint8_t*)mappedResource.pData;
			for (UINT h = 0; h < (UINT)HEIGHT; ++h)
			{
				memcpy(&pData[h * mappedResource.RowPitch], &IMAGE[h * WIDTH * pixelSize], WIDTH * pixelSize);
			}
			pContext->Unmap(pStagingTexture, 0);
		}

	LB_RET:
		return pStagingTexture;
	}

	ID3D11Texture3D* CreateStagingTexture3D(ID3D11Device* pDevice, const int WIDTH, const int HEIGHT, const int DEPTH, const DXGI_FORMAT PIXEL_FORMAT)
	{
		_ASSERT(pDevice != nullptr);

		ID3D11Texture3D* pStagingTexture = nullptr;
		D3D11_TEXTURE3D_DESC textureDesc = { 0, };
		textureDesc.Width = WIDTH;
		textureDesc.Height = HEIGHT;
		textureDesc.Depth = DEPTH;
		textureDesc.MipLevels = 1;
		textureDesc.Format = PIXEL_FORMAT;
		textureDesc.Usage = D3D11_USAGE_STAGING;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;

		pDevice->CreateTexture3D(&textureDesc, nullptr, &pStagingTexture);
		return pStagingTexture;
	}

	HRESULT CreateMetallicRoughnessTexture(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const wchar_t* pszMetallicFileName, const wchar_t* pszRoughnessFileName, ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppSRV)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT(pContext != nullptr);
		_ASSERT((*ppTexture) == nullptr);
		_ASSERT((*ppSRV) == nullptr);

		HRESULT hr = S_OK;

		// GLTF 방식은 이미 합쳐져 있음.
		if (pszMetallicFileName != nullptr && wcscmp(pszMetallicFileName, pszRoughnessFileName) == 0)
		{
			hr = CreateTexture(pDevice, pContext, pszMetallicFileName, false, ppTexture, ppSRV);
		}
		else // 별도의 파일인 경우, 따로 읽어 합쳐줌.
		{
			// ReadImage() 이용.
			// 두 이미지를 4채널로 변환 후, 다시 3채널로 합침.
			int mWidth = 0;
			int mHeight = 0;
			int rWidth = 0;
			int rHeight = 0;
			std::vector<uint8_t> mImage;
			std::vector<uint8_t> rImage;
			std::vector<uint8_t> combinedImage;

			if (pszMetallicFileName != nullptr)
			{
				hr = ReadImage(pszMetallicFileName, mImage, &mWidth, &mHeight);
				if (FAILED(hr))
				{
					goto LB_RET;
				}
			}
			if (pszRoughnessFileName != nullptr)
			{
				hr = ReadImage(pszRoughnessFileName, rImage, &rWidth, &rHeight);
				if (FAILED(hr))
				{
					goto LB_RET;
				}
			}

			// 두 이미지 해상도가 같다고 가정.
			if (pszMetallicFileName != nullptr && pszRoughnessFileName != nullptr)
			{
				_ASSERT(mWidth == rWidth);
				_ASSERT(mHeight == rHeight);
			}

			combinedImage.resize((size_t)(mWidth * mHeight) * 4, 0);
			for (size_t i = 0, size = mWidth * mHeight, mSize = mImage.size(), rSize = rImage.size(); i < size; ++i)
			{
				if (rSize > 0)
				{
					combinedImage[4 * i + 1] = rImage[4 * i]; // Green = Roughness;
				}
				if (mSize > 0)
				{
					combinedImage[4 * i + 2] = mImage[4 * i]; // Blue = Metalness;
				}
			}

			hr = CreateTextureHelper(pDevice, pContext, mWidth, mHeight, combinedImage, DXGI_FORMAT_R8G8B8A8_UNORM, ppTexture, ppSRV);
		}

	LB_RET:
		return hr;
	}

	HRESULT CreateTexture(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const wchar_t* pszFileName, const bool bUSE_SRGB, ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppTextureResourceView)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT(pContext != nullptr);
		_ASSERT((*ppTexture) == nullptr);
		_ASSERT((*ppTextureResourceView) == nullptr);

		HRESULT hr = S_OK;

		int width = 0;
		int height = 0;
		std::vector<uint8_t> image;
		DXGI_FORMAT pixelFormat = (bUSE_SRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);

		if (Utility::GetFileExtension(pszFileName).compare(L"exr") == 0)
		{
			hr = ReadEXRImage(pszFileName, image, &width, &height, &pixelFormat);
		}
		else
		{
			hr = ReadImage(pszFileName, image, &width, &height);
		}

		if (FAILED(hr))
		{
			hr = E_FAIL;
			goto LB_RET;
		}

		hr = CreateTextureHelper(pDevice, pContext, width, height, image, pixelFormat, ppTexture, ppTextureResourceView);

	LB_RET:
		return hr;
	}

	HRESULT CreateTexture(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const wchar_t* pszAlbedoFileName, const wchar_t* pszOpacityFileName, const bool bUSE_SRGB, ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppTextureResourceView)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT(pContext != nullptr);
		_ASSERT((*ppTexture) == nullptr);
		_ASSERT((*ppTextureResourceView) == nullptr);

		HRESULT hr = S_OK;

		int width = 0;
		int height = 0;
		std::vector<uint8_t> image;
		DXGI_FORMAT pixelFormat = (bUSE_SRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);

		hr = ReadImage(pszAlbedoFileName, pszOpacityFileName, image, &width, &height);
		if (FAILED(hr))
		{
			goto LB_RET;
		}

		hr = CreateTextureHelper(pDevice, pContext, width, height, image, pixelFormat, ppTexture, ppTextureResourceView);

	LB_RET:
		return hr;
	}

	HRESULT CreateUATexture(ID3D11Device* pDevice, const int WIDTH, const int HEIGHT, const DXGI_FORMAT PIXEL_FORMAT, ID3D11Texture2D** ppTexture, ID3D11RenderTargetView** ppRTV, ID3D11ShaderResourceView** ppSRV, ID3D11UnorderedAccessView** ppUAV)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT((*ppTexture) == nullptr);
		_ASSERT((*ppRTV) == nullptr);
		_ASSERT((*ppSRV) == nullptr);
		_ASSERT((*ppUAV) == nullptr);

		HRESULT hr = S_OK;

		D3D11_TEXTURE2D_DESC textureDesc = { 0, };
		textureDesc.Width = WIDTH;
		textureDesc.Height = HEIGHT;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = PIXEL_FORMAT; // 주로 float 사용.
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS;

		hr = pDevice->CreateTexture2D(&textureDesc, nullptr, ppTexture);
		if (FAILED(hr))
		{
			goto LB_RET;
		}

		hr = pDevice->CreateRenderTargetView(*ppTexture, nullptr, ppRTV);
		if (FAILED(hr))
		{
			RELEASE(*ppTexture);
			goto LB_RET;
		}

		hr = pDevice->CreateShaderResourceView(*ppTexture, nullptr, ppSRV);
		if (FAILED(hr))
		{
			RELEASE(*ppTexture);
			RELEASE(*ppRTV);
			goto LB_RET;
		}

		hr = pDevice->CreateUnorderedAccessView(*ppTexture, nullptr, ppUAV);
		if (FAILED(hr))
		{
			RELEASE(*ppTexture);
			RELEASE(*ppRTV);
			RELEASE(*ppSRV);
		}

	LB_RET:
		return hr;
	}

	HRESULT CreateTexture3D(ID3D11Device* pDevice, const int WIDTH, const int HEIGHT, const int DEPTH, const DXGI_FORMAT PIXEL_FORMAT, const std::vector<float>& INIT_DATA,
							ID3D11Texture3D** ppTexture, ID3D11RenderTargetView** ppRTV, ID3D11ShaderResourceView** ppSRV, ID3D11UnorderedAccessView** ppUAV)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT((*ppTexture) == nullptr);
		_ASSERT((*ppRTV) == nullptr);
		_ASSERT((*ppSRV) == nullptr);
		_ASSERT((*ppUAV) == nullptr);

		HRESULT hr = S_OK;

		D3D11_TEXTURE3D_DESC textureDesc = { 0, };
		textureDesc.Width = WIDTH;
		textureDesc.Height = HEIGHT;
		textureDesc.Depth = DEPTH;
		textureDesc.MipLevels = 1;
		textureDesc.Format = PIXEL_FORMAT;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS;

		// 3차원 텍스쳐는 x y z 방향 모두 offset 설정해줘야 함.
		if (INIT_DATA.size() > 0)
		{
			size_t pixelSize = GetPixelSize(PIXEL_FORMAT);
			D3D11_SUBRESOURCE_DATA bufferData = { 0, };
			bufferData.pSysMem = INIT_DATA.data();
			bufferData.SysMemPitch = (UINT)(WIDTH * pixelSize);
			bufferData.SysMemSlicePitch = (UINT)(WIDTH * HEIGHT * pixelSize);
			hr = pDevice->CreateTexture3D(&textureDesc, &bufferData, ppTexture);
		}
		else
		{
			hr = pDevice->CreateTexture3D(&textureDesc, nullptr, ppTexture);
		}

		if (FAILED(hr))
		{
			goto LB_RET;
		}

		hr = pDevice->CreateRenderTargetView(*ppTexture, nullptr, ppRTV);
		if (FAILED(hr))
		{
			RELEASE(*ppTexture);
			goto LB_RET;
		}

		hr = pDevice->CreateShaderResourceView(*ppTexture, nullptr, ppSRV);
		if (FAILED(hr))
		{
			RELEASE(*ppTexture);
			RELEASE(*ppRTV);
			goto LB_RET;
		}

		hr = pDevice->CreateUnorderedAccessView(*ppTexture, nullptr, ppUAV);
		if (FAILED(hr))
		{
			RELEASE(*ppTexture);
			RELEASE(*ppRTV);
			RELEASE(*ppSRV);
		}

	LB_RET:
		return hr;
	}

	HRESULT CreateStagingBuffer(ID3D11Device* pDevice, const UINT NUM_ELEMENTS, const UINT SIZE_ELEMENT, const void* pINIT_DATA, ID3D11Buffer** ppBuffer)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT((*ppBuffer) == nullptr);

		HRESULT hr = S_OK;

		D3D11_BUFFER_DESC desc = { 0, };
		desc.ByteWidth = NUM_ELEMENTS * SIZE_ELEMENT;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
		desc.StructureByteStride = SIZE_ELEMENT;

		if (pINIT_DATA)
		{
			D3D11_SUBRESOURCE_DATA bufferData = { 0, };
			bufferData.pSysMem = pINIT_DATA;
			hr = pDevice->CreateBuffer(&desc, &bufferData, ppBuffer);
		}
		else
		{
			hr = pDevice->CreateBuffer(&desc, nullptr, ppBuffer);
		}

		return hr;
	}

	void CopyFromStagingBuffer(ID3D11DeviceContext* pContext, ID3D11Buffer* pBuffer, UINT size, void* pDest)
	{
		_ASSERT(pContext != nullptr);
		_ASSERT(pBuffer != nullptr);
		_ASSERT(pDest != nullptr);

		HRESULT hr = S_OK;

		D3D11_MAPPED_SUBRESOURCE mappedResource = { 0, };
		hr = pContext->Map(pBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);
		memcpy(pDest, mappedResource.pData, size);
		pContext->Unmap(pBuffer, 0);
	}

	void CopyToStagingBuffer(ID3D11DeviceContext* pContext, ID3D11Buffer* pBuffer, UINT size, void* pSrc)
	{
		_ASSERT(pContext != nullptr);
		_ASSERT(pBuffer != nullptr);
		_ASSERT(pSrc != nullptr);

		HRESULT hr = S_OK;

		D3D11_MAPPED_SUBRESOURCE mappedResource = { 0, };
		hr = pContext->Map(pBuffer, NULL, D3D11_MAP_WRITE, NULL, &mappedResource);
		memcpy(mappedResource.pData, pSrc, size);
		pContext->Unmap(pBuffer, NULL);
	}

	HRESULT CreateStructuredBuffer(ID3D11Device* pDevice, const UINT NUM_ELEMENTS, const UINT SIZE_ELEMENT, const void* pINIT_DATA, ID3D11Buffer** ppBuffer, ID3D11ShaderResourceView** ppSRV, ID3D11UnorderedAccessView** ppUAV)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT((*ppBuffer) == nullptr);
		_ASSERT((*ppSRV) == nullptr);
		_ASSERT((*ppUAV) == nullptr);

		HRESULT hr = S_OK;

		D3D11_BUFFER_DESC bufferDesc = { 0, };
		bufferDesc.ByteWidth = NUM_ELEMENTS * SIZE_ELEMENT;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferDesc.StructureByteStride = SIZE_ELEMENT;

		if (pINIT_DATA)
		{
			D3D11_SUBRESOURCE_DATA bufferData = { 0, };
			bufferData.pSysMem = pINIT_DATA;
			hr = pDevice->CreateBuffer(&bufferDesc, &bufferData, ppBuffer);
		}
		else
		{
			hr = pDevice->CreateBuffer(&bufferDesc, nullptr, ppBuffer);
		}

		if (FAILED(hr))
		{
			goto LB_RET;
		}

		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			ZeroMemory(&uavDesc, sizeof(uavDesc));
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.NumElements = NUM_ELEMENTS;

			hr = pDevice->CreateUnorderedAccessView(*ppBuffer, &uavDesc, ppUAV);
			if (FAILED(hr))
			{
				RELEASE(*ppBuffer);
				goto LB_RET;
			}
		}

		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			ZeroMemory(&srvDesc, sizeof(srvDesc));
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			srvDesc.BufferEx.NumElements = NUM_ELEMENTS;

			hr = pDevice->CreateShaderResourceView(*ppBuffer, &srvDesc, ppSRV);
			if (FAILED(hr))
			{
				RELEASE(*ppBuffer);
				RELEASE(*ppUAV);
			}
		}

	LB_RET:
		return hr;
	}

	HRESULT CreateIndirectArgsBuffer(ID3D11Device* pDevice, const UINT NUM_ELEMENTS, const UINT SIZE_ELEMENT, const void* pINIT_DATA, ID3D11Buffer** ppBuffer)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT((*ppBuffer) == nullptr);

		HRESULT hr = S_OK;

		D3D11_BUFFER_DESC desc = { 0, };
		desc.ByteWidth = NUM_ELEMENTS * SIZE_ELEMENT;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
		desc.StructureByteStride = SIZE_ELEMENT;

		if (pINIT_DATA)
		{
			D3D11_SUBRESOURCE_DATA bufferData = { 0, };
			bufferData.pSysMem = pINIT_DATA;

			hr = pDevice->CreateBuffer(&desc, &bufferData, ppBuffer);
		}
		else
		{
			hr = pDevice->CreateBuffer(&desc, nullptr, ppBuffer);
		}

		return hr;
	}

	HRESULT CreateAppendBuffer(ID3D11Device* pDevice, const UINT NUM_ELEMENTS, const UINT SIZE_ELEMENT, const void* pINIT_DATA, ID3D11Buffer** ppBuffer, ID3D11ShaderResourceView** ppSRV, ID3D11UnorderedAccessView** ppUAV)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT((*ppBuffer) == nullptr);
		_ASSERT((*ppSRV) == nullptr);
		_ASSERT((*ppUAV) == nullptr);

		HRESULT hr = S_OK;

		D3D11_BUFFER_DESC bufferDesc = { 0, };
		bufferDesc.ByteWidth = NUM_ELEMENTS * SIZE_ELEMENT;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferDesc.StructureByteStride = SIZE_ELEMENT;

		if (pINIT_DATA)
		{
			D3D11_SUBRESOURCE_DATA bufferData = { 0, };
			bufferData.pSysMem = pINIT_DATA;

			hr = pDevice->CreateBuffer(&bufferDesc, &bufferData, ppBuffer);
		}
		else
		{
			hr = pDevice->CreateBuffer(&bufferDesc, nullptr, ppBuffer);
		}

		if (FAILED(hr))
		{
			goto LB_RET;
		}

		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			ZeroMemory(&uavDesc, sizeof(uavDesc));
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.NumElements = NUM_ELEMENTS;
			uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND; // append buffer로 사용.

			hr = pDevice->CreateUnorderedAccessView(*ppBuffer, &uavDesc, ppUAV);
			if (FAILED(hr))
			{
				RELEASE(*ppBuffer);
				goto LB_RET;
			}
		}

		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			ZeroMemory(&srvDesc, sizeof(srvDesc));
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			srvDesc.BufferEx.NumElements = NUM_ELEMENTS;

			hr = pDevice->CreateShaderResourceView(*ppBuffer, &srvDesc, ppSRV);
			if (FAILED(hr))
			{
				RELEASE(*ppBuffer);
				RELEASE(*ppUAV);
			}
		}

	LB_RET:
		return hr;
	}

	HRESULT CreateTextureArray(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<std::wstring>& FILE_NAMES, ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppTextureResourceView)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT(pContext != nullptr);
		_ASSERT((*ppTexture) == nullptr);
		_ASSERT((*ppTextureResourceView) == nullptr);

		HRESULT hr = S_OK;

		if (FILE_NAMES.empty())
		{
			hr = E_NOTIMPL;
			goto LB_RET;
		}

		// 모든 이미지의 width와 height가 같다고 가정.
		{
			int width = 0;
			int height = 0;
			UINT size = (UINT)(FILE_NAMES.size());
			std::vector<std::vector<uint8_t>> imageArray;

			imageArray.reserve(size);
			for (const std::wstring& f : FILE_NAMES)
			{
				OutputDebugStringW(f.c_str());

				std::vector<uint8_t> image;
				hr = ReadImage(f.c_str(), image, &width, &height);
				if (FAILED(hr))
				{
					goto LB_RET;
				}

				imageArray.push_back(image);
			}

			D3D11_TEXTURE2D_DESC textureDesc = { 0, };
			textureDesc.Width = (UINT)width;
			textureDesc.Height = (UINT)height;
			textureDesc.MipLevels = 0; // 밉맵 레벨 최대.
			textureDesc.ArraySize = size;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Usage = D3D11_USAGE_DEFAULT; // 스테이징 텍스쳐로부터 복사 가능.
			textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
			textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS; // 밉맵 사용.

			hr = pDevice->CreateTexture2D(&textureDesc, nullptr, ppTexture);
			if (FAILED(hr))
			{
				goto LB_RET;
			}

			// pTexture->GetDesc(&textureDesc);

			for (size_t i = 0, size = imageArray.size(); i < size; ++i)
			{
				std::vector<uint8_t>& image = imageArray[i];

				// StagingTexture는 Texture2D임. Texture2DArray가 아님.
				ID3D11Texture2D* pStagingTexture = CreateStagingTexture(pDevice, pContext,
																		width, height,
																		image,
																		textureDesc.Format,
																		1, 1);
				if (pStagingTexture == nullptr)
				{
					goto LB_RET;
				}

				// 스테이징 텍스쳐를 텍스쳐 배열의 해당 위치에 복사.
				UINT subresourceIndex = D3D11CalcSubresource(0, (UINT)i, textureDesc.MipLevels);
				pContext->CopySubresourceRegion(*ppTexture, subresourceIndex, 0, 0, 0, pStagingTexture, 0, nullptr);
			}

			hr = pDevice->CreateShaderResourceView(*ppTexture, nullptr, ppTextureResourceView);
			if (FAILED(hr))
			{
				goto LB_RET;
			}

			pContext->GenerateMips(*ppTextureResourceView);
		}

	LB_RET:
		return hr;
	}

	HRESULT CreateDDSTexture(ID3D11Device* pDevice, const wchar_t* pszFileName, const bool bIsCubeMap, ID3D11ShaderResourceView** ppTextureResourceView)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT((*ppTextureResourceView) == nullptr);

		HRESULT hr = S_OK;

		ID3D11Texture2D* pTexture = nullptr;
		UINT miscFlags = 0;
		if (bIsCubeMap)
		{
			miscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
		}

		hr = DirectX::CreateDDSTextureFromFileEx(pDevice,
												 pszFileName,
												 0,
												 D3D11_USAGE_DEFAULT,
												 D3D11_BIND_SHADER_RESOURCE,
												 0,
												 miscFlags,
												 DirectX::DDS_LOADER_FLAGS(false),
												 (ID3D11Resource**)(&pTexture),
												 ppTextureResourceView,
												 nullptr);
		SAFE_RELEASE(pTexture);
		return hr;
	}

	void WriteToPngFile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ID3D11Texture2D* pTextureToWrite, const wchar_t* pszFileName)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT(pContext != nullptr);
		_ASSERT(pTextureToWrite != nullptr);

		HRESULT hr = S_OK;

		ID3D11Texture2D* pStagingTexture = nullptr;
		D3D11_TEXTURE2D_DESC desc = { 0, };
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = 0;

		hr = pDevice->CreateTexture2D(&desc, nullptr, &pStagingTexture);
		BREAK_IF_FAILED(hr);

		// 전체 복사 시 pContext->CopyResource(pStagingTexture, pTemp);
		// 일부 복사 시 사용.
		D3D11_BOX box = { 0, };
		box.left = 0;
		box.right = desc.Width;
		box.top = 0;
		box.bottom = desc.Height;
		box.front = 0;
		box.back = 1;
		pContext->CopySubresourceRegion(pStagingTexture, 0, 0, 0, 0, pTextureToWrite, 0, &box);

		// R8G8B8A8로 가정.
		std::vector<uint8_t> pixels(desc.Width * desc.Height * 4);
		D3D11_MAPPED_SUBRESOURCE mappedResource = { 0, };
		pContext->Map(pStagingTexture, NULL, D3D11_MAP_READ, NULL, &mappedResource);

		// 텍스쳐가 작을 경우,
		// mappedResource.RowPitch가 width * sizeof(uint8_t) * 4보다 클 수 있어
		// for문을 이용해 가로줄 하나씩 복사.
		uint8_t* pData = (uint8_t*)mappedResource.pData;
		for (uint32_t h = 0; h < desc.Height; ++h)
		{
			memcpy(&pixels[h * desc.Width * 4], &pData[h * mappedResource.RowPitch], desc.Width * sizeof(uint8_t) * 4);
		}
		pContext->Unmap(pStagingTexture, NULL);

		char pFileName[MAX_PATH];
		if (!WideCharToMultiByte(CP_ACP, 0, pszFileName, -1, pFileName, MAX_PATH, nullptr, nullptr))
		{
			pFileName[0] = '\0';
		}

		stbi_write_png(pFileName, desc.Width, desc.Height, 4, pixels.data(), desc.Width * 4);
	}

	size_t GetPixelSize(DXGI_FORMAT pixelFormat)
	{
		switch (pixelFormat)
		{
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return sizeof(uint16_t) * 4;

		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return sizeof(uint32_t) * 4;

		case DXGI_FORMAT_R32_FLOAT:
			return sizeof(uint32_t);

		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return sizeof(uint8_t) * 4;

		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return sizeof(uint8_t) * 4;

		case DXGI_FORMAT_R32_SINT:
			return sizeof(int32_t);

		case DXGI_FORMAT_R16_FLOAT:
			return sizeof(uint16_t);

		default:
			break;
		}

		char debugString[256];
		OutputDebugStringA("PixelFormat not implemented ");
		sprintf(debugString, "%d", pixelFormat);
		OutputDebugStringA(debugString);
		OutputDebugStringA("\n");

		return sizeof(uint8_t) * 4;
	}

} // namespace hlab