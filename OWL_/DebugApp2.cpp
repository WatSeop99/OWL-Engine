#include "Common.h"
#include "Renderer/Renderer.h"
#include "Geometry/Model.h"
#include "Geometry/SkinnedMeshModel.h"
#include "Geometry/GeometryGenerator.h"
#include "Graphics/Light.h"
#include "Geometry/Mesh.h"
#include "Renderer/Timer.h"
#include "Graphics/Scene.h"
#include "Renderer/ResourceManager.h"
#include "Renderer/PostProcessor.h"
#include "DebugApp2.h"

using namespace DirectX::SimpleMath;

DebugApp2::~DebugApp2()
{
	if (m_pGround)
	{
		delete m_pGround;
		m_pGround = nullptr;
	}
	if (m_pScene)
	{
		delete m_pScene;
		m_pScene = nullptr;
	}
	if (m_pRenderer)
	{
		delete m_pRenderer;
		m_pRenderer = nullptr;
	}
}

int DebugApp2::Run()
{
	_ASSERT(m_pRenderer);

	HWND hWnd = m_pRenderer->GetWindowHandle();
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	// 메인 루프.
	MSG msg = { 0, };
	while (msg.message != WM_QUIT && msg.message != WM_DESTROY)
	{
#ifdef PROFILING
		OPTICK_FRAME("MainThread");
#endif
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
#ifdef PROFILING
			OPTICK_EVENT("TranslateMessage");
#endif
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Update(ImGui::GetIO().DeltaTime);
			Render();
		}
	}

	DestroyWindow(hWnd);

	return (int)msg.wParam;
}

bool DebugApp2::Initialize(HINSTANCE hInstance)
{
	bool bRet = true;

	m_pRenderer = new Renderer;
	m_pScene = new Scene;

	if (!m_pRenderer || !m_pRenderer->Initialize(hInstance, m_pScene))
	{
		__debugbreak();
		
		bRet = false;
		goto LB_RET;
	}
	
	if (!m_pScene || !m_pScene->Initialize(m_pRenderer))
	{
		__debugbreak();

		bRet = false;
		goto LB_RET;
	}

	InitScene();

	//m_pRenderer->SetPickedModel(m_pCharacter);
	m_pRenderer->SetPickedModel(m_pScene->RenderObjects[m_pScene->RenderObjects.size() - 2]);

LB_RET:
	return bRet;
}

void DebugApp2::InitScene()
{
	_ASSERT(m_pRenderer);
	_ASSERT(m_pScene);

	HRESULT hr = S_OK;

	m_pRenderer->GetCamera()->Reset(Vector3(3.74966f, 5.03645f, -2.54918f), -0.819048f, 0.741502f);
	m_pRenderer->InitScene();

	{
		/*MeshInfo meshInfo;
		MakeBox(&meshInfo, 0.4f);

		Model* pBox = new Model;
		pBox->Initialize(m_pRenderer, { meshInfo });

		pBox->UpdateWorld(Matrix::CreateTranslation(Vector3(0.5f, 1.0f, 0.2)));

		m_pScene->RenderObjects.push_back(pBox);*/

		MeshInfo meshInfo;
		MakeSphere(&meshInfo, 1.0f, 40, 40);

		Model* pSphere = new Model;
		pSphere->Initialize(m_pRenderer, { meshInfo });

		pSphere->UpdateWorld(Matrix::CreateTranslation(Vector3(0.5f, 1.0f, 0.2)));

		m_pScene->RenderObjects.push_back(pSphere);
	}

	// Main Object.
	{
		std::wstring path = L"./Assets/Characters/Mixamo/";
		std::vector<std::wstring> clipNames =
		{
			L"CatwalkIdle.fbx", L"CatwalkIdleToWalkForward.fbx",
			L"CatwalkWalkForward.fbx", L"CatwalkWalkStop.fbx",
			//L"BreakdanceFreezeVar2.fbx"
		};

		// 로딩 부분 수정해야 함.
		std::wstring filename = L"character.fbx";
		std::vector<MeshInfo> characterMeshInfo;
		AnimationData characterDefaultAnimData;
		hr = ReadFromFile(characterMeshInfo, &characterDefaultAnimData, path, filename);
		BREAK_IF_FAILED(hr);

		// 애니메이션 클립들.
		if (clipNames.size() > 0)
		{
			characterDefaultAnimData.Clips.clear();
		}
		for (SIZE_T i = 0, size = clipNames.size(); i < size; ++i)
		{
			std::wstring& name = clipNames[i];
			AnimationData animDataInClip;

			hr = ReadAnimationFromFile(&animDataInClip, path, name);
			BREAK_IF_FAILED(hr);

			animDataInClip.Clips[0].Name.assign(name.begin(), name.end());
			characterDefaultAnimData.Clips.push_back(animDataInClip.Clips[0]);
		}

		Vector3 center(0.0f, 1.1f, 2.0f);
		m_pCharacter = new SkinnedMeshModel;
		m_pCharacter->Initialize(m_pRenderer, characterMeshInfo, characterDefaultAnimData);
		for (SIZE_T i = 0, size = m_pCharacter->Meshes.size(); i < size; ++i)
		{
			Mesh* pCurMesh = m_pCharacter->Meshes[i];

			MaterialConstants* pMaterialConstData = (MaterialConstants*)pCurMesh->MaterialConstant.pSystemMem;
			pMaterialConstData->AlbedoFactor = Vector3(1.0f);
			pMaterialConstData->RoughnessFactor = 0.8f;
			pMaterialConstData->MetallicFactor = 0.0f;
		}
		m_pCharacter->UpdateWorld(Matrix::CreateTranslation(center));
		m_pCharacter->CharacterAnimationData.Position = center;
		m_pCharacter->bPicked = true;

		m_pScene->RenderObjects.push_back(m_pCharacter); // 리스트에 등록
		m_pScene->pMainController = m_pCharacter;
	}
}

void DebugApp2::Update(float deltaTime)
{
#ifdef PROFILING
	OPTICK_CATEGORY("Update", Optick::Category::Rendering);
#endif

	UpdateGUI();

	m_pScene->Update(deltaTime);
	m_pRenderer->Update(deltaTime);
}

void DebugApp2::Render()
{
	_ASSERT(m_pRenderer);

#ifdef PROFILING
	OPTICK_CATEGORY("Render", Optick::Category::Rendering);
#endif

	//Timer* pTimer = m_pRenderer->GetTimer();

	//pTimer->Start(true);

	m_pRenderer->Render();

	//OutputDebugStringA("Rendering time ==> ");
	//pTimer->End();
}

void DebugApp2::UpdateGUI()
{
	_ASSERT(m_pRenderer);
	_ASSERT(m_pScene);

	ImGui_ImplWin32_NewFrame();
#ifdef DX11
	ImGui_ImplDX11_NewFrame();
#endif

	ImGui::NewFrame();
	//ImGui::DockSpaceOverViewport();

	m_pRenderer->UpdateGUI();
	m_pScene->UpdateGUI();
}
