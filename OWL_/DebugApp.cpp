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
		struct Geometry::MeshData mesh;
		Geometry::MakeSquare(&mesh, 5.0, { 10.0f, 10.0f });
		std::wstring path = L"./Assets/Textures/PBR/stringy-marble-ue/";
		mesh.szAlbedoTextureFileName = path + L"stringy_marble_albedo.png";
		mesh.szEmissiveTextureFileName = L"";
		mesh.szAOTextureFileName = path + L"stringy_marble_ao.png";
		mesh.szMetallicTextureFileName = path + L"stringy_marble_Metallic.png";
		mesh.szNormalTextureFileName = path + L"stringy_marble_Normal-dx.png";
		mesh.szRoughnessTextureFileName = path + L"stringy_marble_Roughness.png";

		m_pGround = New Geometry::Model(m_pDevice5, m_pContext4, { mesh });
		m_pGround->MaterialConstants.CPU.AlbedoFactor = Vector3(0.7f);
		m_pGround->MaterialConstants.CPU.EmissionFactor = Vector3(0.0f);
		m_pGround->MaterialConstants.CPU.MetallicFactor = 0.5f;
		m_pGround->MaterialConstants.CPU.RoughnessFactor = 0.3f;
		m_pGround->Name = "Ground";

		Vector3 position = Vector3(0.0f, -0.5f, 2.0f);
		m_pGround->UpdateWorld(Matrix::CreateRotationX(3.141592f * 0.5f) * Matrix::CreateTranslation(position));

		m_MirrorPlane = SimpleMath::Plane(position, Vector3(0.0f, 1.0f, 0.0f));
		m_pMirror = m_pGround; // 바닥에 거울처럼 반사 구현

		// m_pBasicList.push_back(m_pGround); // 거울은 리스트에 등록 X
	}

	// Main Object.
	{
		// auto meshes = GeometryGenerator::ReadFromFile(
		//     "./Assets/Models/DamagedHelmet/", "DamagedHelmet.gltf");

		// auto meshes = GeometryGenerator::ReadFromFile(
		//     "./Assets/Models/medieval_vagrant_knights/", "scene.gltf",
		//     true);

		// 컴퓨터가 느릴 때는 간단한 물체로 테스트 하세요.
		// vector<Geometry::MeshData> meshes = { Geometry::GeometryGenerator::MakeSphere(0.4f, 50, 50) };

		std::wstring path = L"./Assets/Characters/armored-female-future-soldier/";
		std::wstring fileName = L"angel_armor.fbx";
		std::vector<struct Geometry::MeshData> meshes;
		Geometry::ReadFromFile(meshes, path, fileName);
		meshes[0].szAlbedoTextureFileName = path + L"/angel_armor_albedo.jpg";
		meshes[0].szEmissiveTextureFileName = path + L"/angel_armor_e.jpg";
		meshes[0].szMetallicTextureFileName = path + L"/angel_armor_metalness.jpg";
		meshes[0].szNormalTextureFileName = path + L"/angel_armor_normal.jpg";
		meshes[0].szRoughnessTextureFileName = path + L"/angel_armor_roughness.jpg";

		Vector3 center(0.0f, -0.05f, 2.0f);
		Geometry::Model* newModel = New Geometry::Model(m_pDevice5, m_pContext4, meshes);
		newModel->MaterialConstants.CPU.bInvertNormalMapY = TRUE; // GLTF는 true로.
		newModel->MaterialConstants.CPU.AlbedoFactor = Vector3(1.0f);
		newModel->MaterialConstants.CPU.RoughnessFactor = 0.3f;
		newModel->MaterialConstants.CPU.MetallicFactor = 0.8f;
		newModel->UpdateWorld(Matrix::CreateTranslation(center));
		newModel->bIsPickable = true; // 마우스로 선택/이동 가능.
		newModel->Name = "MainSphere";

		m_pBasicList.push_back(newModel); // 리스트에 등록.
	}

	// 추가 물체1.
	{
		struct Geometry::MeshData mesh;
		Geometry::MakeSphere(&mesh, 0.2f, 200, 200);

		Vector3 center(0.5f, 0.5f, 2.0f);
		Geometry::Model* newModel = New Geometry::Model(m_pDevice5, m_pContext4, { mesh });
		newModel->UpdateWorld(Matrix::CreateTranslation(center));
		newModel->MaterialConstants.CPU.AlbedoFactor = Vector3(0.1f, 0.1f, 1.0f);
		newModel->MaterialConstants.CPU.RoughnessFactor = 0.2f;
		newModel->MaterialConstants.CPU.MetallicFactor = 0.6f;
		newModel->MaterialConstants.CPU.EmissionFactor = Vector3(0.0f);
		newModel->UpdateConstantBuffers(m_pDevice5, m_pContext4);
		newModel->bIsPickable = true; // 마우스로 선택/이동 가능.
		newModel->Name = "SecondSphere";

		m_pBasicList.push_back(newModel);
	}

	// 추가 물체2.
	{
		struct Geometry::MeshData mesh;
		Geometry::MakeBox(&mesh, 0.3f);

		Vector3 center(0.0f, 0.5f, 2.5f);
		Geometry::Model* newModel = New Geometry::Model(m_pDevice5, m_pContext4, { mesh });
		newModel->UpdateWorld(Matrix::CreateTranslation(center));
		newModel->MaterialConstants.CPU.AlbedoFactor = Vector3(1.0f, 0.2f, 0.2f);
		newModel->MaterialConstants.CPU.RoughnessFactor = 0.5f;
		newModel->MaterialConstants.CPU.MetallicFactor = 0.9f;
		newModel->MaterialConstants.CPU.EmissionFactor = Vector3(0.0f);
		newModel->UpdateConstantBuffers(m_pDevice5, m_pContext4);
		newModel->bIsPickable = true; // 마우스로 선택/이동 가능.
		newModel->Name = "Box";

		m_pBasicList.push_back(newModel);
	}
}

void DebugApp::Render()
{
	Core::BaseRenderer::Render();
	Core::BaseRenderer::PostRender();
}

void DebugApp::UpdateGUI()
{
	ImGui::SetNextItemOpen(false, ImGuiCond_Once);
	if (ImGui::TreeNode("General"))
	{
		ImGui::Checkbox("Use FPV", &m_Camera.bUseFirstPersonView);
		ImGui::Checkbox("Wireframe", &m_bDrawAsWire);
		ImGui::Checkbox("DrawOBB", &m_bDrawOBB);
		ImGui::Checkbox("DrawBSphere", &m_bDrawBS);
		if (ImGui::Checkbox("MSAA ON", &m_bUseMSAA))
		{
			createBuffers();
		}
		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Skybox"))
	{
		ImGui::SliderFloat("Strength", &m_GlobalConstsCPU.StrengthIBL, 0.0f, 0.5f);
		ImGui::RadioButton("Env", &m_GlobalConstsCPU.TextureToDraw, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Specular", &m_GlobalConstsCPU.TextureToDraw, 1);
		ImGui::SameLine();
		ImGui::RadioButton("Irradiance", &m_GlobalConstsCPU.TextureToDraw, 2);
		ImGui::SliderFloat("EnvLodBias", &m_GlobalConstsCPU.EnvLODBias, 0.0f, 10.0f);
		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Post Effects"))
	{
		int flag = 0;
		flag += ImGui::RadioButton("Render", &m_PostEffectsConstsCPU.Mode, 1);
		ImGui::SameLine();
		flag += ImGui::RadioButton("Depth", &m_PostEffectsConstsCPU.Mode, 2);
		flag += ImGui::SliderFloat("DepthScale", &m_PostEffectsConstsCPU.DepthScale, 0.0f, 1.0f);
		flag += ImGui::SliderFloat("Fog", &m_PostEffectsConstsCPU.FogStrength, 0.0f, 10.0f);

		if (flag)
		{
			Graphics::UpdateBuffer(m_pContext, m_PostEffectsConstsCPU, m_pPostEffectsConstsGPU);
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Post Processing"))
	{
		int flag = 0;
		flag += ImGui::SliderFloat("Bloom Strength", &m_PostProcess.CombineFilter.ConstantsData.Strength, 0.0f, 1.0f);
		flag += ImGui::SliderFloat("Exposure", &m_PostProcess.CombineFilter.ConstantsData.Option1, 0.0f, 10.0f);
		flag += ImGui::SliderFloat("Gamma", &m_PostProcess.CombineFilter.ConstantsData.Option2, 0.1f, 5.0f);

		// 편의상 사용자 입력이 인식되면 바로 GPU 버퍼를 업데이트.
		if (flag)
		{
			m_PostProcess.CombineFilter.UpdateConstantBuffers(m_pContext);
		}
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

		ImGui::SliderFloat("Metallic", &m_pMirror->MaterialConstants.CPU.MetallicFactor, 0.0f, 1.0f);
		ImGui::SliderFloat("Roughness", &m_pMirror->MaterialConstants.CPU.RoughnessFactor, 0.0f, 1.0f);

		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Light"))
	{
		// ImGui::SliderFloat3("Position", &m_GlobalConstsCPU.lights[0].position.x, -5.0f, 5.0f);
		ImGui::SliderFloat("Halo Radius", &m_GlobalConstsCPU.Lights[1].HaloRadius, 0.0f, 2.0f);
		ImGui::SliderFloat("Halo Strength", &m_GlobalConstsCPU.Lights[1].HaloStrength, 0.0f, 1.0f);
		ImGui::SliderFloat("Radius", &m_GlobalConstsCPU.Lights[1].Radius, 0.0f, 0.5f);
		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Material"))
	{
		ImGui::SliderFloat("LodBias", &m_GlobalConstsCPU.LODBias, 0.0f, 10.0f);

		int flag = 0;

		if (m_pPickedModel)
		{
			flag += ImGui::SliderFloat("Metallic", &m_pPickedModel->MaterialConstants.CPU.MetallicFactor, 0.0f, 1.0f);
			flag += ImGui::SliderFloat("Roughness", &m_pPickedModel->MaterialConstants.CPU.RoughnessFactor, 0.0f, 1.0f);
			flag += ImGui::CheckboxFlags("AlbedoTexture", &m_pPickedModel->MaterialConstants.CPU.bUseAlbedoMap, 1);
			flag += ImGui::CheckboxFlags("EmissiveTexture", &m_pPickedModel->MaterialConstants.CPU.bUseEmissiveMap, 1);
			flag += ImGui::CheckboxFlags("Use NormalMapping", &m_pPickedModel->MaterialConstants.CPU.bUseNormalMap, 1);
			flag += ImGui::CheckboxFlags("Use AO", &m_pPickedModel->MaterialConstants.CPU.bUseAOMap, 1);
			flag += ImGui::CheckboxFlags("Use HeightMapping", &m_pPickedModel->MeshConstants.CPU.bUseHeightMap, 1);
			flag += ImGui::SliderFloat("HeightScale", &m_pPickedModel->MeshConstants.CPU.HeightScale, 0.0f, 0.1f);
			flag += ImGui::CheckboxFlags("Use MetallicMap", &m_pPickedModel->MaterialConstants.CPU.bUseMetallicMap, 1);
			flag += ImGui::CheckboxFlags("Use RoughnessMap", &m_pPickedModel->MaterialConstants.CPU.bUseRoughnessMap, 1);
			if (flag)
			{
				m_pPickedModel->UpdateConstantBuffers(m_pDevice5, m_pContext4);
			}
			ImGui::Checkbox("Draw Normals", &m_pPickedModel->bDrawNormals);
		}

		ImGui::TreePop();
	}
}
