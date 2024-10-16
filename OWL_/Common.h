#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#pragma warning(disable: 4996)

#define INLINE __forceinline
#define VECTOR __vectorcall
#define ALIGN(size) __declspec(align(size))

#include <Windows.h>
//#include <wrl.h>
//#include <wrl/client.h>
//#include <comdef.h>
#include <errhandlingapi.h>
#include <stringapiset.h>
#include <minwindef.h>

#include <d3d11_4.h>
#include <dxgi1_3.h>
#include <dxgi1_5.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include <DirectXPackedVector.h>

#include <assert.h>
#include <float.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <tuple>
#include <map>
#include <algorithm>
#include <filesystem>
#include <unordered_map>
#include <locale>

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

// 수학함수
#include <directxtk/SimpleMath.h>
#include <DirectXCollision.h>

#include "Graphics/EnumData.h"
#include "Util/Util.h"
