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

	m_pRenderer->SetPickedModel(m_pCharacter);

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
		MeshInfo meshInfo;
		MakeBox(&meshInfo, 0.4f);

		Model* pBox = new Model;
		pBox->Initialize(m_pRenderer, { meshInfo });

		pBox->UpdateWorld(Matrix::CreateTranslation(Vector3(0.5f, 1.0f, 0.2)));

		m_pScene->RenderObjects.push_back(pBox);
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

	Camera* pMainCamera = m_pRenderer->GetCamera();
	PostProcessor* pPostProcessor = m_pRenderer->GetPostProcessor();
	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	GlobalConstants* pGlobalConstsCPU = m_pScene->GetGlobalConstantsCPU();

	ImGui_ImplWin32_NewFrame();
#ifdef DX11
	ImGui_ImplDX11_NewFrame();
#endif

	ImGui::NewFrame();
	//ImGui::DockSpaceOverViewport();

	m_pRenderer->UpdateGUI();

	ImGui::Begin("Scene Control");

	ImGui::SetNextItemOpen(false, ImGuiCond_Once);
	if (ImGui::TreeNode("General"))
	{
		ImGui::Checkbox("Use FPV", &pMainCamera->bUseFirstPersonView);
		ImGui::Checkbox("Wireframe", &m_pScene->bDrawAsWire);
		ImGui::Checkbox("DrawOBB", &m_pScene->bDrawOBB);
		ImGui::Checkbox("DrawBSphere", &m_pScene->bDrawBS);
		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Skybox"))
	{
		ImGui::SliderFloat("Strength", &pGlobalConstsCPU->StrengthIBL, 0.0f, 0.5f);
		ImGui::RadioButton("Env", &pGlobalConstsCPU->TextureToDraw, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Specular", &pGlobalConstsCPU->TextureToDraw, 1);
		ImGui::SameLine();
		ImGui::RadioButton("Irradiance", &pGlobalConstsCPU->TextureToDraw, 2);
		ImGui::SliderFloat("EnvLodBias", &pGlobalConstsCPU->EnvLODBias, 0.0f, 10.0f);
		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Post Effects"))
	{
		PostEffectsConstants* pPostEffectConstData = (PostEffectsConstants*)pPostProcessor->GetPostEffectConstantBuffer()->pSystemMem;
		if (!pPostEffectConstData)
		{
			__debugbreak();
		}

		pPostProcessor->PostEffectsUpdateFlag += ImGui::RadioButton("Render", &pPostEffectConstData->Mode, 1);
		ImGui::SameLine();
		pPostProcessor->PostEffectsUpdateFlag += ImGui::RadioButton("Depth", &pPostEffectConstData->Mode, 2);
		pPostProcessor->PostEffectsUpdateFlag += ImGui::SliderFloat("DepthScale", &pPostEffectConstData->DepthScale, 0.0f, 1.0f);
		pPostProcessor->PostEffectsUpdateFlag += ImGui::SliderFloat("Fog", &pPostEffectConstData->FogStrength, 0.0f, 10.0f);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Post Processing"))
	{
		ImageFilterConstData* pCombineFilterConstData = (ImageFilterConstData*)pPostProcessor->CombineFilter.GetConstantBufferPtr()->pSystemMem;
		pPostProcessor->CombineUpdateFlag += ImGui::SliderFloat("Bloom Strength", &pCombineFilterConstData->Strength, 0.0f, 1.0f);
		pPostProcessor->CombineUpdateFlag += ImGui::SliderFloat("Exposure", &pCombineFilterConstData->Option1, 0.0f, 10.0f);
		pPostProcessor->CombineUpdateFlag += ImGui::SliderFloat("Gamma", &pCombineFilterConstData->Option2, 0.1f, 5.0f);
		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Mirror"))
	{
		ImGui::SliderFloat("Alpha", &m_pScene->MirrorAlpha, 0.0f, 1.0f);
		const float BLEND_COLOR[4] = { m_pScene->MirrorAlpha, m_pScene->MirrorAlpha, m_pScene->MirrorAlpha, 1.0f };
		if (m_pScene->bDrawAsWire)
		{
			pResourceManager->GraphicsPSOs[GraphicsPSOType_MirrorBlendWire].SetBlendFactor(BLEND_COLOR);
		}
		else
		{
			pResourceManager->GraphicsPSOs[GraphicsPSOType_MirrorBlendSolid].SetBlendFactor(BLEND_COLOR);
		}

		Model* pMirror = m_pScene->GetMirror();
		MaterialConstants* pMaterialConstData = (MaterialConstants*)pMirror->Meshes[0]->MaterialConstant.pSystemMem;
		if (!pMaterialConstData)
		{
			__debugbreak();
		}

		ImGui::SliderFloat("Metallic", &pMaterialConstData->MetallicFactor, 0.0f, 1.0f);
		ImGui::SliderFloat("Roughness", &pMaterialConstData->RoughnessFactor, 0.0f, 1.0f);

		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Light"))
	{
		ImGui::SliderFloat("Halo Radius", &m_pScene->Lights[1].Property.HaloRadius, 0.0f, 2.0f);
		ImGui::SliderFloat("Halo Strength", &m_pScene->Lights[1].Property.HaloStrength, 0.0f, 1.0f);
		ImGui::SliderFloat("Radius", &m_pScene->Lights[1].Property.Radius, 0.0f, 0.5f);

		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Material"))
	{
		ImGui::SliderFloat("LodBias", &pGlobalConstsCPU->LODBias, 0.0f, 10.0f);

		int flag = 0;
		Model* pPickedModel = m_pRenderer->GetPickedModel();

		if (pPickedModel)
		{
			for (UINT64 i = 0, size = pPickedModel->Meshes.size(); i < size; ++i)
			{
				MaterialConstants* pMaterialConstData = (MaterialConstants*)pPickedModel->Meshes[i]->MaterialConstant.pSystemMem;
				MeshConstants* pMeshConstData = (MeshConstants*)pPickedModel->Meshes[i]->MeshConstant.pSystemMem;
				flag += ImGui::SliderFloat("Metallic", &pMaterialConstData->MetallicFactor, 0.0f, 1.0f);
				flag += ImGui::SliderFloat("Roughness", &pMaterialConstData->RoughnessFactor, 0.0f, 1.0f);
				flag += ImGui::CheckboxFlags("AlbedoTexture", &pMaterialConstData->bUseAlbedoMap, 1);
				flag += ImGui::CheckboxFlags("EmissiveTexture", &pMaterialConstData->bUseEmissiveMap, 1);
				flag += ImGui::CheckboxFlags("Use NormalMapping", &pMaterialConstData->bUseNormalMap, 1);
				flag += ImGui::CheckboxFlags("Use AO", &pMaterialConstData->bUseAOMap, 1);
				flag += ImGui::CheckboxFlags("Use HeightMapping", &pMeshConstData->bUseHeightMap, 1);
				flag += ImGui::SliderFloat("HeightScale", &pMeshConstData->HeightScale, 0.0f, 0.1f);
				flag += ImGui::CheckboxFlags("Use MetallicMap", &pMaterialConstData->bUseMetallicMap, 1);
				flag += ImGui::CheckboxFlags("Use RoughnessMap", &pMaterialConstData->bUseRoughnessMap, 1);
			}

			if (flag)
			{
				pPickedModel->UpdateConstantBuffers();
			}
			ImGui::Checkbox("Draw Normals", &pPickedModel->bDrawNormals);
		}

		ImGui::TreePop();
	}

	ImGui::End();
}
