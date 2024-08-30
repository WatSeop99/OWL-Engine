#include "Common.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "Geometry/GeometryGenerator.h"
#include "Graphics/GraphicsUtils.h"
#include "DebugApp.h"

using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;

DebugApp::DebugApp() : BaseRenderer() { }

DebugApp::~DebugApp()
{
	if (m_pGround)
	{
		delete m_pGround;
		m_pGround = nullptr;
	}
}

void DebugApp::InitScene()
{
	BaseRenderer::m_Camera.Reset(Vector3(-0.112852f, 0.307729f, -0.542159f), 0.0589047f, 0.14399f);

	BaseRenderer::InitScene();

	// Main Object.
	{
		// auto meshes = GeometryGenerator::ReadFromFile(
		//     "./Assets/Models/DamagedHelmet/", "DamagedHelmet.gltf");

		// auto meshes = GeometryGenerator::ReadFromFile(
		//     "./Assets/Models/medieval_vagrant_knights/", "scene.gltf",
		//     true);

		// 컴퓨터가 느릴 때는 간단한 물체로 테스트 하세요.
		// vector<Geometry::MeshInfo> meshes = { Geometry::GeometryGenerator::MakeSphere(0.4f, 50, 50) };

		std::wstring path = L"./Assets/Characters/armored-female-future-soldier/";
		std::wstring fileName = L"angel_armor.fbx";
		std::vector<MeshInfo> meshInfos;
		ReadFromFile(meshInfos, path, fileName);
		meshInfos[0].szAlbedoTextureFileName = path + L"/angel_armor_albedo.jpg";
		meshInfos[0].szEmissiveTextureFileName = path + L"/angel_armor_e.jpg";
		meshInfos[0].szMetallicTextureFileName = path + L"/angel_armor_metalness.jpg";
		meshInfos[0].szNormalTextureFileName = path + L"/angel_armor_normal.jpg";
		meshInfos[0].szRoughnessTextureFileName = path + L"/angel_armor_roughness.jpg";

		Vector3 center(0.0f, -0.05f, 2.0f);
		Model* newModel = New Model;
		newModel->Initialize(m_pDevice, m_pContext, meshInfos);

		MaterialConstants* pMaterialConstData = (MaterialConstants*)newModel->Meshes[0]->MaterialConstant.pSystemMem;
		pMaterialConstData->bInvertNormalMapY = TRUE;
		pMaterialConstData->AlbedoFactor = Vector3(1.0f);
		pMaterialConstData->RoughnessFactor = 0.3f;
		pMaterialConstData->MetallicFactor = 0.8f;

		newModel->UpdateWorld(Matrix::CreateTranslation(center));
		newModel->bIsPickable = true; // 마우스로 선택/이동 가능.
		newModel->Name = "MainModel";

		m_Scene.pRenderObjects.push_back(newModel); // 리스트에 등록.
	}

	// 추가 물체1.
	{
		MeshInfo meshInfo;
		MakeSphere(&meshInfo, 0.2f, 200, 200);

		Vector3 center(0.5f, 0.5f, 2.0f);
		Model* newModel = New Model;
		newModel->Initialize(m_pDevice, m_pContext, { meshInfo });
		newModel->UpdateWorld(Matrix::CreateTranslation(center));
		for (UINT64 i = 0, size = newModel->Meshes.size(); i < size; ++i)
		{
			Mesh* pCurMesh = newModel->Meshes[i];

			MaterialConstants* pMaterialConstData = (MaterialConstants*)pCurMesh->MaterialConstant.pSystemMem;
			pMaterialConstData->AlbedoFactor = Vector3(0.1f, 0.1f, 1.0f);
			pMaterialConstData->EmissionFactor = Vector3(0.0f);
			pMaterialConstData->MetallicFactor = 0.6f;
			pMaterialConstData->RoughnessFactor = 0.2f;
		}
		newModel->UpdateConstantBuffers(m_pContext);
		newModel->bIsPickable = true; // 마우스로 선택/이동 가능.
		newModel->Name = "SecondSphere";

		m_Scene.pRenderObjects.push_back(newModel);
	}

	// 추가 물체2.
	{
		MeshInfo meshInfo;
		MakeBox(&meshInfo, 0.3f);

		Vector3 center(0.0f, 0.5f, 2.5f);
		Model* newModel = New Model;
		newModel->Initialize(m_pDevice, m_pContext, { meshInfo });
		newModel->UpdateWorld(Matrix::CreateTranslation(center));
		for (UINT64 i = 0, size = newModel->Meshes.size(); i < size; ++i)
		{
			Mesh* pCurMesh = newModel->Meshes[i];
			
			MaterialConstants* pMaterialConstData = (MaterialConstants*)pCurMesh->MaterialConstant.pSystemMem;
			pMaterialConstData->AlbedoFactor = Vector3(1.0f, 0.2f, 0.2f);
			pMaterialConstData->EmissionFactor = Vector3(0.0f);
			pMaterialConstData->MetallicFactor = 0.9f;
			pMaterialConstData->RoughnessFactor = 0.5f;
		}
		newModel->UpdateConstantBuffers(m_pContext);
		newModel->bIsPickable = true; // 마우스로 선택/이동 가능.
		newModel->Name = "Box";

		m_Scene.pRenderObjects.push_back(newModel);
	}

	// 추가 물체3.
	{
		MeshInfo meshInfo;
		MakeSphere(&meshInfo, 0.2f, 200, 200);

		Vector3 center(0.5f, 0.2f, -1.0f);
		Model* newModel = New Model;
		newModel->Initialize(m_pDevice, m_pContext, { meshInfo });
		newModel->UpdateWorld(Matrix::CreateTranslation(center));
		for (UINT64 i = 0, size = newModel->Meshes.size(); i < size; ++i)
		{
			Mesh* pCurMesh = newModel->Meshes[i];

			MaterialConstants* pMaeterialConstData = (MaterialConstants*)pCurMesh->MaterialConstant.pSystemMem;
			pMaeterialConstData->AlbedoFactor = Vector3(0.1f, 0.1f, 1.0f);
			pMaeterialConstData->EmissionFactor = Vector3(0.0f);
			pMaeterialConstData->MetallicFactor = 0.6f;
			pMaeterialConstData->RoughnessFactor = 0.2f;
		}
		newModel->UpdateConstantBuffers(m_pContext);
		newModel->bIsPickable = true; // 마우스로 선택/이동 가능.
		newModel->Name = "SecondSphere";

		m_Scene.pRenderObjects.push_back(newModel);
	}
}

void DebugApp::Render()
{
	m_pTimer->Start(m_pContext, true);

	BaseRenderer::Render();

	OutputDebugStringA("Rendering time ==> ");
	m_pTimer->End(m_pContext);
}

void DebugApp::UpdateGUI()
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
			destroyBuffersForRendering();
			createBuffers();
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
		// 갱신된 내용은 Update()에서 처리.
		m_PostProcessor.PostEffectsUpdateFlag += ImGui::RadioButton("Render", &(m_PostProcessor.PostEffectsConstsCPU.Mode), 1);
		ImGui::SameLine();
		m_PostProcessor.PostEffectsUpdateFlag += ImGui::RadioButton("Depth", &(m_PostProcessor.PostEffectsConstsCPU.Mode), 2);
		m_PostProcessor.PostEffectsUpdateFlag += ImGui::SliderFloat("DepthScale", &(m_PostProcessor.PostEffectsConstsCPU.DepthScale), 0.0f, 1.0f);
		m_PostProcessor.PostEffectsUpdateFlag += ImGui::SliderFloat("Fog", &(m_PostProcessor.PostEffectsConstsCPU.FogStrength), 0.0f, 10.0f);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Post Processing"))
	{
		// 갱신된 내용은 Update()에서 처리.
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
		const float blendColor[4] = { m_Scene.MirrorAlpha, m_Scene.MirrorAlpha, m_Scene.MirrorAlpha, 1.0f };
		if (m_Scene.bDrawAsWire)
		{
			g_MirrorBlendWirePSO.SetBlendFactor(blendColor);
		}
		else
		{
			g_MirrorBlendSolidPSO.SetBlendFactor(blendColor);
		}

		Model* pMirror = m_Scene.GetMirror();
		MaterialConstants* pMaterialConstData = (MaterialConstants*)pMirror->Meshes[0]->MaterialConstant.pSystemMem;
		ImGui::SliderFloat("Metallic", &pMaterialConstData->MetallicFactor, 0.0f, 1.0f);
		ImGui::SliderFloat("Roughness", &pMaterialConstData->RoughnessFactor, 0.0f, 1.0f);

		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Light"))
	{
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
				MeshConstants* pMeshConstData = (MeshConstants*)m_pPickedModel->Meshes[i]->MeshConstant.pSystemMem;
				MaterialConstants* pMaterialConstData = (MaterialConstants*)m_pPickedModel->Meshes[i]->MaterialConstant.pSystemMem;

				flag += ImGui::SliderFloat("Metallic", &pMaterialConstData->MetallicFactor, 0.0f, 1.0f);
				flag += ImGui::SliderFloat("Roughness", &pMaterialConstData->RoughnessFactor, 0.0f, 1.0f);
				flag += ImGui::CheckboxFlags("AlbedoTexture", &pMaterialConstData->bUseAlbedoMap, TRUE);
				flag += ImGui::CheckboxFlags("EmissiveTexture", &pMaterialConstData->bUseEmissiveMap, TRUE);
				flag += ImGui::CheckboxFlags("Use NormalMapping", &pMaterialConstData->bUseNormalMap, TRUE);
				flag += ImGui::CheckboxFlags("Use AO", &pMaterialConstData->bUseAOMap, 1);
				flag += ImGui::CheckboxFlags("Use HeightMapping", &pMeshConstData->bUseHeightMap, TRUE);
				flag += ImGui::SliderFloat("HeightScale", &pMeshConstData->HeightScale, 0.0f, 0.1f);
				flag += ImGui::CheckboxFlags("Use MetallicMap", &pMaterialConstData->bUseMetallicMap, TRUE);
				flag += ImGui::CheckboxFlags("Use RoughnessMap", &pMaterialConstData->bUseRoughnessMap, TRUE);
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
