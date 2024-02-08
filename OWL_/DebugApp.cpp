#include "Common.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "Geometry/GeometryGenerator.h"
#include "Core/GraphicsUtils.h"
#include "DebugApp.h"

using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;

DebugApp::DebugApp() : Core::BaseRenderer() { }

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
	Core::BaseRenderer::m_Camera.Reset(Vector3(-0.112852f, 0.307729f, -0.542159f), 0.0589047f, 0.14399f);
	Core::BaseRenderer::m_GlobalConstsCPU.StrengthIBL = 0.1f;

	Core::BaseRenderer::InitCubemaps(L"./Assets/Textures/Cubemaps/HDRI/",
									 L"SampleEnvHDR.dds", L"SampleSpecularHDR.dds",
									 L"SampleDiffuseHDR.dds", L"SampleBrdf.dds");

	Core::BaseRenderer::InitScene();

	// 바닥(거울)
	{
		// https://freepbr.com/materials/stringy-marble-pbr/
		Geometry::MeshInfo meshInfo = INIT_MESH_INFO;
		Geometry::MakeSquare(&meshInfo, 5.0, { 10.0f, 10.0f });
		std::wstring path = L"./Assets/Textures/PBR/stringy-marble-ue/";
		meshInfo.szAlbedoTextureFileName = path + L"stringy_marble_albedo.png";
		meshInfo.szEmissiveTextureFileName = L"";
		meshInfo.szAOTextureFileName = path + L"stringy_marble_ao.png";
		meshInfo.szMetallicTextureFileName = path + L"stringy_marble_Metallic.png";
		meshInfo.szNormalTextureFileName = path + L"stringy_marble_Normal-dx.png";
		meshInfo.szRoughnessTextureFileName = path + L"stringy_marble_Roughness.png";

		m_pGround = New Geometry::Model(m_pDevice5, m_pContext4, { meshInfo });
		m_pGround->pMeshes[0]->MaterialConstants.CPU.AlbedoFactor = Vector3(0.7f);
		m_pGround->pMeshes[0]->MaterialConstants.CPU.EmissionFactor = Vector3(0.0f);
		m_pGround->pMeshes[0]->MaterialConstants.CPU.MetallicFactor = 0.5f;
		m_pGround->pMeshes[0]->MaterialConstants.CPU.RoughnessFactor = 0.3f;
		m_pGround->Name = "Ground";

		Vector3 position = Vector3(0.0f, -0.5f, 2.0f);
		m_pGround->UpdateWorld(Matrix::CreateRotationX(DirectX::XM_PI * 0.5f) * Matrix::CreateTranslation(position));

		m_MirrorPlane = SimpleMath::Plane(position, Vector3(0.0f, 1.0f, 0.0f));
		m_pMirror = m_pGround; // 바닥에 거울처럼 반사 구현.

		// 거울은 리스트에 등록 X.
	}

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
		std::vector<Geometry::MeshInfo> meshInfos;
		Geometry::ReadFromFile(meshInfos, path, fileName);
		meshInfos[0].szAlbedoTextureFileName = path + L"/angel_armor_albedo.jpg";
		meshInfos[0].szEmissiveTextureFileName = path + L"/angel_armor_e.jpg";
		meshInfos[0].szMetallicTextureFileName = path + L"/angel_armor_metalness.jpg";
		meshInfos[0].szNormalTextureFileName = path + L"/angel_armor_normal.jpg";
		meshInfos[0].szRoughnessTextureFileName = path + L"/angel_armor_roughness.jpg";

		Vector3 center(0.0f, -0.05f, 2.0f);
		Geometry::Model* newModel = New Geometry::Model(m_pDevice5, m_pContext4, meshInfos);
		newModel->pMeshes[0]->MaterialConstants.CPU.bInvertNormalMapY = TRUE; // GLTF는 true로.
		newModel->pMeshes[0]->MaterialConstants.CPU.AlbedoFactor = Vector3(1.0f);
		newModel->pMeshes[0]->MaterialConstants.CPU.RoughnessFactor = 0.3f;
		newModel->pMeshes[0]->MaterialConstants.CPU.MetallicFactor = 0.8f;
		newModel->UpdateWorld(Matrix::CreateTranslation(center));
		newModel->bIsPickable = true; // 마우스로 선택/이동 가능.
		newModel->Name = "MainSphere";

		m_pBasicList.push_back(newModel); // 리스트에 등록.
	}

	// 추가 물체1.
	{
		Geometry::MeshInfo meshInfo = INIT_MESH_INFO;
		Geometry::MakeSphere(&meshInfo, 0.2f, 200, 200);

		Vector3 center(0.5f, 0.5f, 2.0f);
		Geometry::Model* newModel = New Geometry::Model(m_pDevice5, m_pContext4, { meshInfo });
		newModel->UpdateWorld(Matrix::CreateTranslation(center));
		for (size_t i = 0, size = newModel->pMeshes.size(); i < size; ++i)
		{
			Geometry::Mesh* pCurMesh = newModel->pMeshes[i];
			pCurMesh->MaterialConstants.CPU.AlbedoFactor = Vector3(0.1f, 0.1f, 1.0f);
			pCurMesh->MaterialConstants.CPU.EmissionFactor = Vector3(0.0f);
			pCurMesh->MaterialConstants.CPU.MetallicFactor = 0.6f;
			pCurMesh->MaterialConstants.CPU.RoughnessFactor = 0.2f;
		}
		newModel->UpdateConstantBuffers(m_pDevice5, m_pContext4);
		newModel->bIsPickable = true; // 마우스로 선택/이동 가능.
		newModel->Name = "SecondSphere";

		m_pBasicList.push_back(newModel);
	}

	// 추가 물체2.
	{
		Geometry::MeshInfo meshInfo = INIT_MESH_INFO;
		Geometry::MakeBox(&meshInfo, 0.3f);

		Vector3 center(0.0f, 0.5f, 2.5f);
		Geometry::Model* newModel = New Geometry::Model(m_pDevice5, m_pContext4, { meshInfo });
		newModel->UpdateWorld(Matrix::CreateTranslation(center));
		for (size_t i = 0, size = newModel->pMeshes.size(); i < size; ++i)
		{
			Geometry::Mesh* pCurMesh = newModel->pMeshes[i];
			pCurMesh->MaterialConstants.CPU.AlbedoFactor = Vector3(1.0f, 0.2f, 0.2f);
			pCurMesh->MaterialConstants.CPU.EmissionFactor = Vector3(0.0f);
			pCurMesh->MaterialConstants.CPU.MetallicFactor = 0.9f;
			pCurMesh->MaterialConstants.CPU.RoughnessFactor = 0.5f;
		}
		newModel->UpdateConstantBuffers(m_pDevice5, m_pContext4);
		newModel->bIsPickable = true; // 마우스로 선택/이동 가능.
		newModel->Name = "Box";

		m_pBasicList.push_back(newModel);
	}
}

void DebugApp::Render()
{
	Core::BaseRenderer::Render();
}

void DebugApp::UpdateGUI()
{
	BaseRenderer::UpdateGUI();

	ImGui::SetNextItemOpen(false, ImGuiCond_Once);
	if (ImGui::TreeNode("General"))
	{
		ImGui::Checkbox("Use FPV", &(m_Camera.bUseFirstPersonView));
		ImGui::Checkbox("Wireframe", &m_bDrawAsWire);
		ImGui::Checkbox("DrawOBB", &m_bDrawOBB);
		ImGui::Checkbox("DrawBSphere", &m_bDrawBS);
		if (ImGui::Checkbox("MSAA ON", &m_bUseMSAA))
		{
			destroyBuffersForRendering();
			createBuffers();
			m_PostProcessor.Initialize(m_pDevice5, m_pContext4,
									   { m_pGlobalConstsGPU, m_pBackBuffer, m_FloatBuffer.GetTexture(), m_ResolvedBuffer.GetTexture(), m_PrevBuffer.GetTexture(), m_pBackBufferRTV, m_ResolvedBuffer.GetSRV(), m_PrevBuffer.GetSRV(), m_DepthOnlyBuffer.GetSRV() },
									   m_ScreenWidth, m_ScreenHeight, 4);
		}
		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Skybox"))
	{
		ImGui::SliderFloat("Strength", &(m_GlobalConstsCPU.StrengthIBL), 0.0f, 0.5f);
		ImGui::RadioButton("Env", &(m_GlobalConstsCPU.TextureToDraw), 0);
		ImGui::SameLine();
		ImGui::RadioButton("Specular", &(m_GlobalConstsCPU.TextureToDraw), 1);
		ImGui::SameLine();
		ImGui::RadioButton("Irradiance", &(m_GlobalConstsCPU.TextureToDraw), 2);
		ImGui::SliderFloat("EnvLodBias", &(m_GlobalConstsCPU.EnvLODBias), 0.0f, 10.0f);
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
		m_PostProcessor.CombineUpdateFlag += ImGui::SliderFloat("Bloom Strength", &(m_PostProcessor.CombineFilter.ConstantsData.Strength), 0.0f, 1.0f);
		m_PostProcessor.CombineUpdateFlag += ImGui::SliderFloat("Exposure", &(m_PostProcessor.CombineFilter.ConstantsData.Option1), 0.0f, 10.0f);
		m_PostProcessor.CombineUpdateFlag += ImGui::SliderFloat("Gamma", &(m_PostProcessor.CombineFilter.ConstantsData.Option2), 0.1f, 5.0f);

		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Mirror"))
	{
		ImGui::SliderFloat("Alpha", &m_MirrorAlpha, 0.0f, 1.0f);
		const float blendColor[4] = { m_MirrorAlpha, m_MirrorAlpha, m_MirrorAlpha, 1.0f };
		if (m_bDrawAsWire)
		{
			Graphics::g_MirrorBlendWirePSO.SetBlendFactor(blendColor);
		}
		else
		{
			Graphics::g_MirrorBlendSolidPSO.SetBlendFactor(blendColor);
		}

		ImGui::SliderFloat("Metallic", &(m_pMirror->pMeshes[0]->MaterialConstants.CPU.MetallicFactor), 0.0f, 1.0f);
		ImGui::SliderFloat("Roughness", &(m_pMirror->pMeshes[0]->MaterialConstants.CPU.RoughnessFactor), 0.0f, 1.0f);

		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Light"))
	{
		//// ImGui::SliderFloat3("Position", &m_GlobalConstsCPU.lights[0].position.x, -5.0f, 5.0f);
		/*ImGui::SliderFloat("Halo Radius", &(m_GlobalConstsCPU.Lights[1].HaloRadius), 0.0f, 2.0f);
		ImGui::SliderFloat("Halo Strength", &(m_GlobalConstsCPU.Lights[1].HaloStrength), 0.0f, 1.0f);
		ImGui::SliderFloat("Radius", &(m_GlobalConstsCPU.Lights[1].Radius), 0.0f, 0.5f);*/
		ImGui::SliderFloat("Halo Radius", &(m_pLights[1].Property.HaloRadius), 0.0f, 2.0f);
		ImGui::SliderFloat("Halo Strength", &(m_pLights[1].Property.HaloStrength), 0.0f, 1.0f);
		ImGui::SliderFloat("Radius", &(m_pLights[1].Property.Radius), 0.0f, 0.5f);
		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Material"))
	{
		ImGui::SliderFloat("LodBias", &m_GlobalConstsCPU.LODBias, 0.0f, 10.0f);

		int flag = 0;

		if (m_pPickedModel)
		{
			for (size_t i = 0, size = m_pPickedModel->pMeshes.size(); i < size; ++i)
			{
				flag += ImGui::SliderFloat("Metallic", &(m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.MetallicFactor), 0.0f, 1.0f);
				flag += ImGui::SliderFloat("Roughness", &(m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.RoughnessFactor), 0.0f, 1.0f);
				flag += ImGui::CheckboxFlags("AlbedoTexture", &(m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.bUseAlbedoMap), TRUE);
				flag += ImGui::CheckboxFlags("EmissiveTexture", &(m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.bUseEmissiveMap), TRUE);
				flag += ImGui::CheckboxFlags("Use NormalMapping", &m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.bUseNormalMap, TRUE);
				flag += ImGui::CheckboxFlags("Use AO", &(m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.bUseAOMap), 1);
				flag += ImGui::CheckboxFlags("Use HeightMapping", &(m_pPickedModel->pMeshes[i]->MeshConstants.CPU.bUseHeightMap), TRUE);
				flag += ImGui::SliderFloat("HeightScale", &(m_pPickedModel->pMeshes[i]->MeshConstants.CPU.HeightScale), 0.0f, 0.1f);
				flag += ImGui::CheckboxFlags("Use MetallicMap", &(m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.bUseMetallicMap), TRUE);
				flag += ImGui::CheckboxFlags("Use RoughnessMap", &(m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.bUseRoughnessMap), TRUE);
			}
			if (flag)
			{
				m_pPickedModel->UpdateConstantBuffers(m_pDevice5, m_pContext4);
			}
			ImGui::Checkbox("Draw Normals", &m_pPickedModel->bDrawNormals);
		}

		ImGui::TreePop();
	}

	ImGui::End();
}
