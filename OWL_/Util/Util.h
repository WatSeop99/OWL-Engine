#pragma once

#include <intsafe.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <iostream>
#include <utility>

#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define DBG_MALLOC(size) _malloc_dbg((size), _NORMAL_BLOCK, __FILE__, __LINE__)
#define Malloc(size) DBG_MALLOC(size)

#ifdef __cplusplus
#define DBG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define New DBG_NEW
#endif

#define BREAK_IF_FAILED(hr) \
    if (FAILED(hr))         \
    {                       \
        __debugbreak();     \
    }
#define SET_DEBUG_INFO_TO_OBJECT(ptr, name)  (ptr)->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(name) - 1, (name))

#else

#define Malloc(size) malloc(size)
#ifdef __cplusplus
#define New new
#endif

#define BREAK_IF_FAILED(hr)
#define SET_DEBUG_INFO_TO_OBJECT(ptr, name)

#endif

#define SAFE_RELEASE(x)	\
	if (x)				\
	{					\
		(x)->Release();	\
		(x) = nullptr;	\
	}

#define RELEASE(x)      \
    {                   \
        (x)->Release(); \
        (x) = nullptr;  \
    }

namespace Utility
{
    std::wstring UTF8ToWideString(const std::string& str);
    std::string WideStringToUTF8(const std::wstring& wstr);
    std::string ToLower(const std::string& str);
    std::wstring ToLower(const std::wstring& str);
    std::string GetBasePath(const std::string& str);
    std::wstring GetBasePath(const std::wstring& str);
    std::string RemoveBasePath(const std::string& str);
    std::wstring RemoveBasePath(const std::wstring& str);
    std::string GetFileExtension(const std::string& str);
    std::wstring GetFileExtension(const std::wstring& str);
    std::string RemoveExtension(const std::string& str);
    std::wstring RemoveExtension(const std::wstring& str);
}
