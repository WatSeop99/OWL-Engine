#include "Common.h"
#include "DebugApp.h"
#include "DebugApp2.h"

using namespace std;

#ifdef _DEBUG

#include <dxgidebug.h>

void CheckD3DMemoryLeak()
{
	HMODULE dxgiDebugDLL = GetModuleHandleW(L"dxgidebug.dll");
	decltype(&DXGIGetDebugInterface) GetDebugInterface = (decltype(&DXGIGetDebugInterface))(GetProcAddress(dxgiDebugDLL, "DXGIGetDebugInterface"));

	IDXGIDebug* pDebug = nullptr;

	GetDebugInterface(IID_PPV_ARGS(&pDebug));

	OutputDebugStringW(L"================================== Direct3D Object Memory Leak List ==================================\n");
	pDebug->ReportLiveObjects(DXGI_DEBUG_D3D11, DXGI_DEBUG_RLO_DETAIL);
	OutputDebugStringW(L"======================================================================================================\n");

	pDebug->Release();
}

#endif

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// std::unique_ptr<hlab::BaseRenderer> app = make_unique<hlab::DebugApp>();
	Core::BaseRenderer* app = New DebugApp2();
	app->Initialize();
	app->Run();

	if (app)
	{
		delete app;
		app = nullptr;
	}

#ifdef _DEBUG
	CheckD3DMemoryLeak();
	_ASSERT(_CrtCheckMemory());
#endif

	return 0;
}