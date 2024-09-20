#pragma once

#include <intsafe.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <directxtk/SimpleMath.h>

#include <iostream>
#include <utility>
#include <string>

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Quaternion;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector2;

#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define DBG_MALLOC(size) _malloc_dbg((size), _NORMAL_BLOCK, __FILE__, __LINE__)
#define Malloc(size) DBG_MALLOC(size)

#ifdef __cplusplus
#define DBG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define New DBG_NEW
#endif

#define BREAK_IF_FAILED(hr) if (FAILED(hr)) { __debugbreak(); }
#define SET_DEBUG_INFO_TO_OBJECT(ptr, name) if (ptr) { (ptr)->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(name) - 1, (name)); }

#else

#define Malloc(size) malloc(size)
#ifdef __cplusplus
#define New new
#endif

#define BREAK_IF_FAILED(hr)
#define SET_DEBUG_INFO_TO_OBJECT(ptr, name)

#endif

#define SAFE_RELEASE(p)	if (p) { (p)->Release(); (p) = nullptr; }
#define RELEASE(p) { (p)->Release(); (p) = nullptr; }


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

int Min(const int X, const int Y);
float Min(const float X, const float Y);
Vector3 Min(const Vector3& V1, const Vector3& V2);

int Max(const int X, const int Y);
float Max(const float X, const float Y);
Vector3 Max(const Vector3& V1, const Vector3& V2);

float Clamp(const float VAL, const float LOWER, const float UPPER);
float Lerp(const float A, const float B, const float F);
float DegreeToRadian(const float ANGLE);
