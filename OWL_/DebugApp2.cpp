#include "Common.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "Geometry/GeometryGenerator.h"
#include "DebugApp2.h"

using namespace DirectX::SimpleMath;

DebugApp2::~DebugApp2()
{
	if (m_pGround)
	{
		delete m_pGround;
		m_pGround = nullptr;
	}
}

void DebugApp2::InitScene()
{
	BaseRenderer::m_Camera.Reset(Vector3(3.74966f, 5.03645f, -2.54918f), -0.819048f, 0.741502f);

	BaseRenderer::InitScene();

	// Main Object.
	{
		std::wstring path = L"./Assets/Characters/Mixamo/";
		std::vector<std::wstring> clipNames =
		{
			L"CatwalkIdle.fbx", L"CatwalkIdleToWalkForward.fbx",
			L"CatwalkWalkForward.fbx", L"CatwalkWalkStop.fbx",
			L"BreakdanceFreezeVar2.fbx"
		};
		AnimationData aniData;

		std::wstring filename = L"character.fbx";
		std::tuple<std::vector<MeshInfo>, AnimationData> data;
		ReadAnimationFromFile(data, path, filename);
		std::vector<MeshInfo>& meshInfos = std::get<0>(data);

		for (UINT64 i = 0, size = clipNames.size(); i < size; ++i)
		{
			std::wstring& name = clipNames[i];
			std::tuple<std::vector<MeshInfo>, AnimationData> tempData;
			ReadAnimationFromFile(tempData, path, name);
			AnimationData& anim = std::get<1>(tempData);

			if (aniData.Clips.empty())
			{
				aniData = anim;
			}
			else
			{
				aniData.Clips.push_back(anim.Clips[0]);
			}
		}

		Vector3 center(0.0f, 0.0f, 2.0f);
		m_pCharacter = New SkinnedMeshModel;
		m_pCharacter->Initialize(m_pDevice, m_pContext, meshInfos, aniData);
		for (UINT64 i = 0, size = m_pCharacter->Meshes.size(); i < size; ++i)
		{
			Mesh* pCurMesh = m_pCharacter->Meshes[i];

			MaterialConstants* pMaterialConstData = (MaterialConstants*)pCurMesh->MaterialConstant.pSystemMem;
			/*pCurMesh->MaterialConstant.CPU.AlbedoFactor = Vector3(1.0f);
			pCurMesh->MaterialConstant.CPU.RoughnessFactor = 0.8f;
			pCurMesh->MaterialConstant.CPU.MetallicFactor = 0.0f;*/
			pMaterialConstData->AlbedoFactor = Vector3(1.0f);
			pMaterialConstData->RoughnessFactor = 0.8f;
			pMaterialConstData->MetallicFactor = 0.0f;
		}
		m_pCharacter->UpdateWorld(Matrix::CreateScale(1.0f) * Matrix::CreateTranslation(center));

		m_Scene.pRenderObjects.push_back(m_pCharacter); // 리스트에 등록
		m_pPickedModel = m_pCharacter;
	}
}

void DebugApp2::UpdateGUI()
{
	BaseRenderer::UpdateGUI();
	GlobalConstants* pGlobalConstsCPU = m_Scene.GetGlobalConstantsCPU();

	ImGui::SetNextItemOpen(false, ImGuiCond_Once);
	if (ImGui::TreeNode("General"))
	{
		ImGui::Checkbox("Use FPV", &(m_Camera.bUseFirstPersonView));
		ImGui::Checkbox("Wireframe", &(m_Scene.bDrawAsWire));
		ImGui::Checkbox("DrawOBB", &(m_Scene.bDrawOBB));
		ImGui::Checkbox("DrawBSphere", &(m_Scene.bDrawBS));
		if (ImGui::Checkbox("MSAA ON", &m_bUseMSAA))
		{
			BaseRenderer::destroyBuffersForRendering();
			BaseRenderer::createBuffers();
			m_PostProcessor.Initialize(m_pDevice, m_pContext,
									   { m_Scene.GetGlobalConstantsGPU(), m_pBackBuffer, m_FloatBuffer.pTexture, m_ResolvedBuffer.pTexture, m_PrevBuffer.pTexture, m_pBackBufferRTV, m_ResolvedBuffer.pSRV, m_PrevBuffer.pSRV, m_Scene.GetDepthOnlyBufferSRV() },
									   m_ScreenWidth, m_ScreenHeight, 4);
		}
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
		m_PostProcessor.PostEffectsUpdateFlag += ImGui::RadioButton("Render", &(m_PostProcessor.PostEffectsConstsCPU.Mode), 1);
		ImGui::SameLine();
		m_PostProcessor.PostEffectsUpdateFlag += ImGui::RadioButton("Depth", &(m_PostProcessor.PostEffectsConstsCPU.Mode), 2);
		m_PostProcessor.PostEffectsUpdateFlag += ImGui::SliderFloat("DepthScale", &(m_PostProcessor.PostEffectsConstsCPU.DepthScale), 0.0f, 1.0f);
		m_PostProcessor.PostEffectsUpdateFlag += ImGui::SliderFloat("Fog", &(m_PostProcessor.PostEffectsConstsCPU.FogStrength), 0.0f, 10.0f);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Post Processing"))
	{
		ImageFilterConstData* pCombineFilterConstData = (ImageFilterConstData*)m_PostProcessor.CombineFilter.GetConstantBufferPtr()->pSystemMem;
		m_PostProcessor.CombineUpdateFlag += ImGui::SliderFloat("Bloom Strength", &pCombineFilterConstData->Strength, 0.0f, 1.0f);
		m_PostProcessor.CombineUpdateFlag += ImGui::SliderFloat("Exposure", &pCombineFilterConstData->Option1, 0.0f, 10.0f);
		m_PostProcessor.CombineUpdateFlag += ImGui::SliderFloat("Gamma", &pCombineFilterConstData->Option2, 0.1f, 5.0f);
		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Mirror"))
	{
		ImGui::SliderFloat("Alpha", &(m_Scene.MirrorAlpha), 0.0f, 1.0f);
		const float BLEND_COLOR[4] = { m_Scene.MirrorAlpha, m_Scene.MirrorAlpha, m_Scene.MirrorAlpha, 1.0f };
		if (m_Scene.bDrawAsWire)
		{
			g_MirrorBlendWirePSO.SetBlendFactor(BLEND_COLOR);
		}
		else
		{
			g_MirrorBlendSolidPSO.SetBlendFactor(BLEND_COLOR);
		}

		Model* pMirror = m_Scene.GetMirror();
		MaterialConstants* pMaterialConstData = (MaterialConstants*)pMirror->Meshes[0]->MaterialConstant.pSystemMem;
		ImGui::SliderFloat("Metallic", &pMaterialConstData->MetallicFactor, 0.0f, 1.0f);
		ImGui::SliderFloat("Roughness", &pMaterialConstData->RoughnessFactor, 0.0f, 1.0f);
		/*ImGui::SliderFloat("Metallic", &(pMirror->Meshes[0]->MaterialConstant.CPU.MetallicFactor), 0.0f, 1.0f);
		ImGui::SliderFloat("Roughness", &(pMirror->Meshes[0]->MaterialConstant.CPU.RoughnessFactor), 0.0f, 1.0f);*/

		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Light"))
	{
		// ImGui::SliderFloat3("Position", &m_globalConstsCPU.lights[0].position.x, -5.0f, 5.0f);
		ImGui::SliderFloat("Halo Radius", &(m_Scene.pLights[1].Property.HaloRadius), 0.0f, 2.0f);
		ImGui::SliderFloat("Halo Strength", &(m_Scene.pLights[1].Property.HaloStrength), 0.0f, 1.0f);
		ImGui::SliderFloat("Radius", &(m_Scene.pLights[1].Property.Radius), 0.0f, 0.5f);

		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Material"))
	{
		ImGui::SliderFloat("LodBias", &pGlobalConstsCPU->LODBias, 0.0f, 10.0f);

		int flag = 0;

		if (m_pPickedModel)
		{
			for (UINT64 i = 0, size = m_pPickedModel->Meshes.size(); i < size; ++i)
			{
				MaterialConstants* pMaterialConstData = (MaterialConstants*)m_pPickedModel->Meshes[i]->MaterialConstant.pSystemMem;
				MeshConstants* pMeshConstData = (MeshConstants*)m_pPickedModel->Meshes[i]->MeshConstant.pSystemMem;
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

				/*flag += ImGui::SliderFloat("Metallic", &(m_pPickedModel->Meshes[i]->MaterialConstant.CPU.MetallicFactor), 0.0f, 1.0f);
				flag += ImGui::SliderFloat("Roughness", &(m_pPickedModel->Meshes[i]->MaterialConstant.CPU.RoughnessFactor), 0.0f, 1.0f);
				flag += ImGui::CheckboxFlags("AlbedoTexture", &(m_pPickedModel->Meshes[i]->MaterialConstant.CPU.bUseAlbedoMap), 1);
				flag += ImGui::CheckboxFlags("EmissiveTexture", &(m_pPickedModel->Meshes[i]->MaterialConstant.CPU.bUseEmissiveMap), 1);
				flag += ImGui::CheckboxFlags("Use NormalMapping", &(m_pPickedModel->Meshes[i]->MaterialConstant.CPU.bUseNormalMap), 1);
				flag += ImGui::CheckboxFlags("Use AO", &(m_pPickedModel->Meshes[i]->MaterialConstant.CPU.bUseAOMap), 1);
				flag += ImGui::CheckboxFlags("Use HeightMapping", &(m_pPickedModel->Meshes[i]->MeshConstant.CPU.bUseHeightMap), 1);
				flag += ImGui::SliderFloat("HeightScale", &(m_pPickedModel->Meshes[i]->MeshConstant.CPU.HeightScale), 0.0f, 0.1f);
				flag += ImGui::CheckboxFlags("Use MetallicMap", &(m_pPickedModel->Meshes[i]->MaterialConstant.CPU.bUseMetallicMap), 1);
				flag += ImGui::CheckboxFlags("Use RoughnessMap", &(m_pPickedModel->Meshes[i]->MaterialConstant.CPU.bUseRoughnessMap), 1);*/
			}

			if (flag)
			{
				m_pPickedModel->UpdateConstantBuffers(m_pContext);
			}
			ImGui::Checkbox("Draw Normals", &(m_pPickedModel->bDrawNormals));
		}

		ImGui::TreePop();
	}

	ImGui::End();
}

void DebugApp2::Update(float deltaTime)
{
	BaseRenderer::Update(deltaTime);

	static int s_FrameCount = 0;

	// States
	// 0: idle
	// 1: idle to walk
	// 2: walk forward
	// 3: walk to stop
	// 4: dance
	static int s_State = 0;

	switch (s_State)
	{
	case 0:
	{
		if (BaseRenderer::m_pbKeyPressed[VK_UP])
		{
			s_State = 1;
			s_FrameCount = 0;
		}
		else if (s_FrameCount ==
				 m_pCharacter->CharacterAnimaionData.Clips[s_State].Keys[0].size() ||
				 BaseRenderer::m_pbKeyPressed[VK_UP]) // 재생이 다 끝난다면.
		{
			s_FrameCount = 0; // 상태 변화 없이 반복.
		}
	}
		break;

	case 1:
	{
		if (s_FrameCount == m_pCharacter->CharacterAnimaionData.Clips[s_State].Keys[0].size())
		{
			s_State = 2;
			s_FrameCount = 0;
		}
	}
		break;

	case 2:
	{
		if (BaseRenderer::m_pbKeyPressed[VK_RIGHT])
		{
			m_pCharacter->CharacterAnimaionData.AccumulatedRootTransform =
				Matrix::CreateRotationY(DirectX::XM_PI * 60.0f / 180.0f * deltaTime) *
				m_pCharacter->CharacterAnimaionData.AccumulatedRootTransform;
		}
		if (BaseRenderer::m_pbKeyPressed[VK_LEFT])
		{
			m_pCharacter->CharacterAnimaionData.AccumulatedRootTransform =
				Matrix::CreateRotationY(-DirectX::XM_PI * 60.0f / 180.0f * deltaTime) *
				m_pCharacter->CharacterAnimaionData.AccumulatedRootTransform;
		}
		if (s_FrameCount == m_pCharacter->CharacterAnimaionData.Clips[s_State].Keys[0].size())
		{
			// 방향키를 누르고 있지 않으면 정지. (누르고 있으면 계속 걷기)
			if (!BaseRenderer::m_pbKeyPressed[VK_UP])
			{
				s_State = 3;
			}
			s_FrameCount = 0;
		}
	}
		break;

	case 3:
	{
		if (s_FrameCount == m_pCharacter->CharacterAnimaionData.Clips[s_State].Keys[0].size())
		{
			// s_State = 4;
			s_State = 0;
			s_FrameCount = 0;
		}
	}
		break;

	case 4:
	{
		if (s_FrameCount == m_pCharacter->CharacterAnimaionData.Clips[s_State].Keys[0].size())
		{
			s_State = 0;
			s_FrameCount = 0;
		}
	}
		break;

	default:
		break;
	}

	m_pCharacter->UpdateAnimation(m_pContext, s_State, s_FrameCount);

	++s_FrameCount;
}

void DebugApp2::Render()
{
	m_pTimer->Start(m_pContext, true);

	BaseRenderer::Render();

	OutputDebugStringA("Rendering time ==> ");
	m_pTimer->End(m_pContext);
}
