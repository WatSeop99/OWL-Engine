#pragma once

HRESULT ReadEXRImage(const WCHAR* pszFileName, std::vector<UINT8>& image, int* pWidth, int* pHeight, DXGI_FORMAT* pPixelFormat);
HRESULT ReadImage(const WCHAR* pszFileName, std::vector<UINT8>& image, int* pWidth, int* pHeight, DXGI_FORMAT* pPixelFormat, const bool bUSE_SRGB = false);
HRESULT ReadImage(const WCHAR* pszAlbedoFileName, const WCHAR* pszOpacityFileName, std::vector<UINT8>& image, int* pWidth, int* pHeight);

UINT64 GetPixelSize(DXGI_FORMAT pixelFormat);
