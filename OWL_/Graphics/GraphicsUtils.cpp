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

HRESULT ReadEXRImage(const WCHAR* pszFileName, std::vector<UINT8>& image, int* pWidth, int* pHeight, DXGI_FORMAT* pPixelFormat)
{
	_ASSERT(pszFileName);
	_ASSERT(pWidth);
	_ASSERT(pHeight);
	_ASSERT(pPixelFormat);

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

	*pWidth = (int)metaData.width;
	*pHeight = (int)metaData.height;
	*pPixelFormat = metaData.format;

	image.resize(scratchImage.GetPixelsSize());
	memcpy(image.data(), scratchImage.GetPixels(), image.size());

	// Debug. 데이터 범위 확인.
	/*
	std::vector<float> f32;
	uint16_t* pF16 = nullptr;
	float minValue = FLT_MAX;
	float maxValue = FLT_MIN;
	f32.resize(image.SIZE() / 2);
	pF16 = (uint16_t*)image.data();
	for (int i = 0, SIZE = image.SIZE() / 2; i < SIZE; ++i)
	{
		f32[i] = fp16_ieee_to_fp32_value(f16[i]);
		minValue = (minValue < f32[i] ? minValue : f32[i]);
		maxValue = (maxValue > f32[i] ? maxValue : f32[i]);
	}*/

LB_RET:
	return hr;
}

HRESULT ReadImage(const WCHAR* pszFileName, std::vector<UINT8>& image, int* pWidth, int* pHeight, DXGI_FORMAT* pPixelFormat, const bool bUSE_SRGB)
{
	_ASSERT(pszFileName);
	_ASSERT(pWidth);
	_ASSERT(pHeight);
	_ASSERT(pPixelFormat);

	HRESULT hr = S_OK;
	int channels = 0;

	char pFileName[256];
	if (!WideCharToMultiByte(CP_ACP, 0, pszFileName, -1, pFileName, MAX_PATH, nullptr, nullptr))
	{
		pFileName[0] = '\0';
	}

	BYTE* pImg = stbi_load(pFileName, pWidth, pHeight, &channels, 0);
	if (!pImg)
	{
		__debugbreak();
	}

	// 4채널로 만들어 복사.
	image.resize((*pWidth) * (*pHeight) * 4, 255);
	switch (channels)
	{
		case 1:
			for (int i = 0, size = (*pWidth) * (*pHeight); i < size; ++i)
			{
				UINT8 g = pImg[i * channels];
				for (int c = 0; c < 4; ++c)
				{
					image[4 * i + c] = g;
				}
			}
			break;

		case 2:
			for (int i = 0, size = (*pWidth) * (*pHeight); i < size; ++i)
			{
				for (int c = 0; c < 2; ++c)
				{
					image[4 * i + c] = pImg[i * channels + c];
				}
			}
			break;

		case 3:
			for (int i = 0, size = (*pWidth) * (*pHeight); i < size; ++i)
			{
				for (int c = 0; c < 3; ++c)
				{
					image[4 * i + c] = pImg[i * channels + c];
				}
			}
			break;

		case 4:
			for (int i = 0, size = (*pWidth) * (*pHeight); i < size; ++i)
			{
				for (int c = 0; c < 4; ++c)
				{
					image[4 * i + c] = pImg[i * channels + c];
				}
			}
			break;

		default:
			hr = E_FAIL;
			break;
	}

	free(pImg);

//	DirectX::TexMetadata metaData;
//	DirectX::ScratchImage scratchImage;
//	DirectX::ScratchImage convertImage;
//
//	std::wstring fileExtension = GetFileExtension(pszFileName);
//
//	if (fileExtension.compare(L"hdr") == 0)
//	{
//		hr = GetMetadataFromHDRFile(pszFileName, metaData);
//		if (FAILED(hr))
//		{
//			goto LB_RET;
//		}
//
//		hr = LoadFromHDRFile(pszFileName, nullptr, scratchImage);
//		if (FAILED(hr))
//		{
//			goto LB_RET;
//		}
//	}
//	else if (fileExtension.compare(L"tga") == 0)
//	{
//		hr = GetMetadataFromTGAFile(pszFileName, metaData);
//		if (FAILED(hr))
//		{
//			goto LB_RET;
//		}
//
//		hr = LoadFromTGAFile(pszFileName, nullptr, scratchImage);
//		if (FAILED(hr))
//		{
//			goto LB_RET;
//		}
//	}
//	else
//	{
//		//hr = GetMetadataFromWICFile(pszFileName, DirectX::WIC_FLAGS_NONE, metaData);
//		hr = GetMetadataFromWICFile(pszFileName, bUSE_SRGB ? DirectX::WIC_FLAGS_FORCE_RGB : DirectX::WIC_FLAGS_DEFAULT_SRGB, metaData);
//		if (FAILED(hr))
//		{
//			goto LB_RET;
//		}
//
//		//hr = LoadFromWICFile(pszFileName, DirectX::WIC_FLAGS_NONE, &metaData, scratchImage);
//		hr = LoadFromWICFile(pszFileName, bUSE_SRGB ? DirectX::WIC_FLAGS_FORCE_RGB : DirectX::WIC_FLAGS_DEFAULT_SRGB, &metaData, scratchImage);
//		if (FAILED(hr))
//		{
//			goto LB_RET;
//		}
//	}
//
//	*pWidth = (int)metaData.width;
//	*pHeight = (int)metaData.height;
//	*pPixelFormat = metaData.format;
//	image.resize(scratchImage.GetPixelsSize(), 255);
//
//	if (DirectX::IsBGR(*pPixelFormat))
//	{
//		DirectX::Convert(scratchImage.GetImages(), scratchImage.GetImageCount(), scratchImage.GetMetadata(), DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, convertImage);
//		memcpy(image.data(), convertImage.GetPixels(), image.size());
//	}
//	else
//	{
//		memcpy(image.data(), scratchImage.GetPixels(), image.size());
//	}
//
//LB_RET:
	return hr;
}

HRESULT ReadImage(const WCHAR* pszAlbedoFileName, const WCHAR* pszOpacityFileName, std::vector<UINT8>& image, int* pWidth, int* pHeight)
{
	std::vector<UINT8> opacityImage;
	DXGI_FORMAT albedoFormat;
	HRESULT hr = ReadImage(pszAlbedoFileName, image, pWidth, pHeight, &albedoFormat, true);
	if (FAILED(hr))
	{
		goto LB_RET;
	}


	int opaWidth = 0;
	int opaHeight = 0;
	DXGI_FORMAT opacityFormat;
	hr = ReadImage(pszOpacityFileName, opacityImage, &opaWidth, &opaHeight, &opacityFormat, true);
	if (FAILED(hr))
	{
		goto LB_RET;
	}

	_ASSERT(*pWidth == opaWidth && *pHeight == opaHeight);

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


UINT64 GetPixelSize(DXGI_FORMAT pixelFormat)
{
	switch (pixelFormat)
	{
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return sizeof(UINT16) * 4;

		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return sizeof(UINT32) * 4;

		case DXGI_FORMAT_R32_FLOAT:
			return sizeof(UINT32);

		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return sizeof(UINT8) * 4;

		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return sizeof(UINT8) * 4;

		case DXGI_FORMAT_R32_SINT:
			return sizeof(int);

		case DXGI_FORMAT_R16_FLOAT:
			return sizeof(UINT16);

		default:
			break;
	}

	char szDebugString[256];
	sprintf_s(szDebugString, 256, "pixelFormat not implemented %d\n", pixelFormat);
	OutputDebugStringA(szDebugString);

	return sizeof(UINT8) * 4;
}
