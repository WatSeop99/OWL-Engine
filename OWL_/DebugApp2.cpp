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
	m_GlobalConstsCPU.StrengthIBL = 1.0f;
	Core::BaseRenderer::m_Camera.Reset(Vector3(3.74966f, 5.03645f, -2.54918f), -0.819048f, 0.741502f);

	Core::BaseRenderer::InitCubemaps(L"./Assets/Textures/Cubemaps/HDRI/", L"clear_pureskyEnvHDR.dds",
									 L"clear_pureskySpecularHDR.dds", L"clear_pureskyDiffuseHDR.dds",
									 L"clear_pureskyBrdf.dds");

	Core::BaseRenderer::InitScene();

	// 조명 설정.
	{
		// 조명 0은 고정.
		/*m_GlobalConstsCPU.Lights[0].Radiance = Vector3(5.0f);
		m_GlobalConstsCPU.Lights[0].Position = Vector3(0.0f, 2.0f, 2.0f);
		m_GlobalConstsCPU.Lights[0].Direction = Vector3(0.2f, -1.0f, 0.0f);
		m_GlobalConstsCPU.Lights[0].SpotPower = 0.0f;
		m_GlobalConstsCPU.Lights[0].Radius = 0.1f;
		m_GlobalConstsCPU.Lights[0].Type = LIGHT_POINT | LIGHT_SHADOW;

		m_GlobalConstsCPU.Lights[1].Type = LIGHT_OFF;
		m_GlobalConstsCPU.Lights[2].Type = LIGHT_OFF;*/
		m_pLights[0].Property.Radiance = Vector3(5.0f);
		m_pLights[0].Property.Position = Vector3(0.0f, 2.0f, 2.0f);
		m_pLights[0].Property.Direction = Vector3(0.2f, -1.0f, 0.0f);
		m_pLights[0].Property.SpotPower = 0.0f;
		m_pLights[0].Property.Radius = 0.1f;
		m_pLights[0].Property.LightType = LIGHT_POINT | LIGHT_SHADOW;

		m_pLights[1].Property.LightType = LIGHT_OFF;
		m_pLights[2].Property.LightType = LIGHT_OFF;
	}

	// 바닥(거울).
	{
		// https://freepbr.com/materials/stringy-marble-pbr/
		Geometry::MeshInfo mesh = INIT_MESH_INFO;
		Geometry::MakeSquare(&mesh, 5.0f);

		std::wstring path = L"./Assets/Textures/PBR/stringy-marble-ue/";
		mesh.szAlbedoTextureFileName = path + L"stringy_marble_albedo.png";
		mesh.szEmissiveTextureFileName = L"";
		mesh.szAOTextureFileName = path + L"stringy_marble_ao.png";
		mesh.szMetallicTextureFileName = path + L"stringy_marble_Metallic.png";
		mesh.szNormalTextureFileName = path + L"stringy_marble_Normal-dx.png";
		mesh.szRoughnessTextureFileName = path + L"stringy_marble_Roughness.png";

		m_pGround = New Geometry::Model(m_pDevice5, m_pContext4, { mesh });
		m_pGround->pMeshes[0]->MaterialConstants.CPU.AlbedoFactor = Vector3(0.7f);
		m_pGround->pMeshes[0]->MaterialConstants.CPU.EmissionFactor = Vector3(0.0f);
		m_pGround->pMeshes[0]->MaterialConstants.CPU.MetallicFactor = 0.5f;
		m_pGround->pMeshes[0]->MaterialConstants.CPU.RoughnessFactor = 0.3f;

		Vector3 position = Vector3(0.0f, 0.0f, 2.0f);
		m_pGround->UpdateWorld(Matrix::CreateRotationX(DirectX::XM_PI * 0.5f) *
							   Matrix::CreateTranslation(position));
		m_pGround->bCastShadow = false; // 바닥은 그림자 만들기 생략.

		m_MirrorPlane = DirectX::SimpleMath::Plane(position, Vector3(0.0f, 1.0f, 0.0f));
		m_pMirror = m_pGround; // 바닥에 거울처럼 반사 구현.

		// m_basicList.push_back(m_ground); // 거울은 리스트에 등록 X
	}

	// Main Object.
	{
		std::wstring path = L"./Assets/Characters/Mixamo/";
		std::vector<std::wstring> clipNames =
		{
			L"CatwalkIdle.fbx", L"CatwalkIdleToWalkForward.fbx",
			L"CatwalkWalkForward.fbx", L"CatwalkWalkStop.fbx",
			L"BreakdanceFreezeVar2.fbx"
		};
		Geometry::AnimationData aniData;

		std::wstring filename = L"character.fbx";
		std::tuple<std::vector<Geometry::MeshInfo>, Geometry::AnimationData> data;
		Geometry::ReadAnimationFromFile(data, path, filename);
		std::vector<Geometry::MeshInfo>& meshInfos = std::get<0>(data);

		for (size_t i = 0, size = clipNames.size(); i < size; ++i)
		{
			std::wstring& name = clipNames[i];
			std::tuple<std::vector<Geometry::MeshInfo>, Geometry::AnimationData> tempData;
			Geometry::ReadAnimationFromFile(tempData, path, name);
			Geometry::AnimationData& anim = std::get<1>(tempData);

			if (aniData.pClips.empty())
			{
				aniData = anim;
			}
			else
			{
				aniData.pClips.push_back(anim.pClips[0]);
			}
		}

		Vector3 center(0.0f, 0.5f, 2.0f);
		m_pCharacter = New Geometry::SkinnedMeshModel(m_pDevice5, m_pContext4, meshInfos, aniData);
		/*m_pCharacter->MaterialConstants.CPU.AlbedoFactor = Vector3(1.0f);
		m_pCharacter->MaterialConstants.CPU.RoughnessFactor = 0.8f;
		m_pCharacter->MaterialConstants.CPU.MetallicFactor = 0.0f;*/
		for (size_t i = 0, size = m_pCharacter->pMeshes.size(); i < size; ++i)
		{
			Geometry::Mesh* pCurMesh = m_pCharacter->pMeshes[i];
			pCurMesh->MaterialConstants.CPU.AlbedoFactor = Vector3(1.0f);
			pCurMesh->MaterialConstants.CPU.RoughnessFactor = 0.8f;
			pCurMesh->MaterialConstants.CPU.MetallicFactor = 0.0f;
		}
		m_pCharacter->UpdateWorld(Matrix::CreateScale(1.0f) * Matrix::CreateTranslation(center));

		m_pBasicList.push_back(m_pCharacter); // 리스트에 등록
		m_pPickedModel = m_pCharacter;
	}
}

void DebugApp2::UpdateGUI()
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
			Core::BaseRenderer::destroyBuffersForRendering();
			Core::BaseRenderer::createBuffers();
			m_PostProcessor.Initialize(m_pDevice5, m_pContext4,
									   { m_pGlobalConstsGPU, m_pBackBuffer, m_FloatBuffer.GetTexture(), m_ResolvedBuffer.GetTexture(), m_PrevBuffer.GetTexture(), m_pBackBufferRTV, m_ResolvedBuffer.GetSRV(), m_PrevBuffer.GetSRV(), m_DepthOnlyBuffer.GetSRV() },
									   m_ScreenWidth, m_ScreenHeight, 4);
		}
		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Skybox"))
	{
		ImGui::SliderFloat("Strength", &(m_GlobalConstsCPU.StrengthIBL), 0.0f,
						   0.5f);
		ImGui::RadioButton("Env", &(m_GlobalConstsCPU.TextureToDraw), 0);
		ImGui::SameLine();
		ImGui::RadioButton("Specular", &m_GlobalConstsCPU.TextureToDraw, 1);
		ImGui::SameLine();
		ImGui::RadioButton("Irradiance", &(m_GlobalConstsCPU.TextureToDraw), 2);
		ImGui::SliderFloat("EnvLodBias", &(m_GlobalConstsCPU.EnvLODBias), 0.0f, 10.0f);
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
		m_PostProcessor.CombineUpdateFlag += ImGui::SliderFloat("Bloom Strength", &(m_PostProcessor.CombineFilter.ConstantsData.Strength), 0.0f, 1.0f);
		m_PostProcessor.CombineUpdateFlag += ImGui::SliderFloat("Exposure", &(m_PostProcessor.CombineFilter.ConstantsData.Option1), 0.0f, 10.0f);
		m_PostProcessor.CombineUpdateFlag += ImGui::SliderFloat("Gamma", &(m_PostProcessor.CombineFilter.ConstantsData.Option2), 0.1f, 5.0f);

		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Mirror"))
	{
		ImGui::SliderFloat("Alpha", &m_MirrorAlpha, 0.0f, 1.0f);
		const float BLEND_COLOR[4] = { m_MirrorAlpha, m_MirrorAlpha, m_MirrorAlpha, 1.0f };
		if (m_bDrawAsWire)
		{
			Graphics::g_MirrorBlendWirePSO.SetBlendFactor(BLEND_COLOR);
		}
		else
		{
			Graphics::g_MirrorBlendSolidPSO.SetBlendFactor(BLEND_COLOR);
		}

		ImGui::SliderFloat("Metallic", &(m_pMirror->pMeshes[0]->MaterialConstants.CPU.MetallicFactor), 0.0f, 1.0f);
		ImGui::SliderFloat("Roughness", &(m_pMirror->pMeshes[0]->MaterialConstants.CPU.RoughnessFactor), 0.0f, 1.0f);

		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Light"))
	{
		// ImGui::SliderFloat3("Position",
		// &m_globalConstsCPU.lights[0].position.x, -5.0f, 5.0f);
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
		ImGui::SliderFloat("LodBias", &(m_GlobalConstsCPU.LODBias), 0.0f, 10.0f);

		int flag = 0;

		if (m_pPickedModel != nullptr)
		{
			for (size_t i = 0, size = m_pPickedModel->pMeshes.size(); i < size; ++i)
			{
				flag += ImGui::SliderFloat("Metallic", &(m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.MetallicFactor), 0.0f, 1.0f);
				flag += ImGui::SliderFloat("Roughness", &(m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.RoughnessFactor), 0.0f, 1.0f);
				flag += ImGui::CheckboxFlags("AlbedoTexture", &(m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.bUseAlbedoMap), 1);
				flag += ImGui::CheckboxFlags("EmissiveTexture", &(m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.bUseEmissiveMap), 1);
				flag += ImGui::CheckboxFlags("Use NormalMapping", &(m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.bUseNormalMap), 1);
				flag += ImGui::CheckboxFlags("Use AO", &(m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.bUseAOMap), 1);
				flag += ImGui::CheckboxFlags("Use HeightMapping", &(m_pPickedModel->pMeshes[i]->MeshConstants.CPU.bUseHeightMap), 1);
				flag += ImGui::SliderFloat("HeightScale", &(m_pPickedModel->pMeshes[i]->MeshConstants.CPU.HeightScale), 0.0f, 0.1f);
				flag += ImGui::CheckboxFlags("Use MetallicMap", &(m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.bUseMetallicMap), 1);
				flag += ImGui::CheckboxFlags("Use RoughnessMap", &(m_pPickedModel->pMeshes[i]->MaterialConstants.CPU.bUseRoughnessMap), 1);
			}

			if (flag)
			{
				m_pPickedModel->UpdateConstantBuffers(m_pDevice5, m_pContext4);
			}
			ImGui::Checkbox("Draw Normals", &(m_pPickedModel->bDrawNormals));
		}

		ImGui::TreePop();
	}

	ImGui::End();
}

void DebugApp2::Update(float deltaTime)
{
	Core::BaseRenderer::Update(deltaTime);

	static int s_FrameCount = 0;

	// States
	// 0: idle
	// 1: idle to walk
	// 2: walk forward
	// 3: walk to stop
	// 4: dance
	static int s_State = 0;

	// 주의: s_FrameCount = 0;

	if (s_State == 0) // 정지 상태.
	{
		if (Core::BaseRenderer::m_pbKeyPressed[VK_UP])
		{
			s_State = 1;
			s_FrameCount = 0;
		}
		else if (s_FrameCount ==
				 m_pCharacter->m_AnimData.pClips[s_State].pKeys[0].size() ||
				 Core::BaseRenderer::m_pbKeyPressed[VK_UP]) // 재생이 다 끝난다면.
		{
			s_FrameCount = 0; // 상태 변화 없이 반복.
		}
	}
	else if (s_State == 1)
	{
		if (s_FrameCount == m_pCharacter->m_AnimData.pClips[s_State].pKeys[0].size())
		{
			s_State = 2;
			s_FrameCount = 0;
		}
	}
	else if (s_State == 2)
	{
		if (Core::BaseRenderer::m_pbKeyPressed[VK_RIGHT])
		{
			m_pCharacter->m_AnimData.AccumulatedRootTransform =
				Matrix::CreateRotationY(DirectX::XM_PI * 60.0f / 180.0f * deltaTime) *
				m_pCharacter->m_AnimData.AccumulatedRootTransform;
		}
		if (Core::BaseRenderer::m_pbKeyPressed[VK_LEFT])
		{
			m_pCharacter->m_AnimData.AccumulatedRootTransform =
				Matrix::CreateRotationY(-DirectX::XM_PI * 60.0f / 180.0f * deltaTime) *
				m_pCharacter->m_AnimData.AccumulatedRootTransform;
		}
		if (s_FrameCount == m_pCharacter->m_AnimData.pClips[s_State].pKeys[0].size())
		{
			// 방향키를 누르고 있지 않으면 정지. (누르고 있으면 계속 걷기)
			if (!Core::BaseRenderer::m_pbKeyPressed[VK_UP])
			{
				s_State = 3;
			}
			s_FrameCount = 0;
		}
	}
	else if (s_State == 3)
	{
		if (s_FrameCount == m_pCharacter->m_AnimData.pClips[s_State].pKeys[0].size())
		{
			s_State = 4;
			s_FrameCount = 0;
		}
	}
	else if (s_State == 4)
	{
		if (s_FrameCount == m_pCharacter->m_AnimData.pClips[s_State].pKeys[0].size())
		{
			s_State = 0;
			s_FrameCount = 0;
		}
	}

	m_pCharacter->UpdateAnimation(m_pContext4, s_State, s_FrameCount);

	++s_FrameCount;
}

void DebugApp2::Render()
{
	m_pTimer->Start(m_pContext4, true);

	Core::BaseRenderer::Render();

	OutputDebugStringA("Rendering time ==> ");
	m_pTimer->End(m_pContext4);
}
