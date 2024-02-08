#pragma once

#include "Model.h"
#include "../Core/StructuredBuffer.h"

namespace Geometry
{
	class SkinnedMeshModel : public Model
	{
	public:
		SkinnedMeshModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<MeshInfo>& MESHES, const AnimationData& ANIM_DATA);
		~SkinnedMeshModel() = default;
		
		void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<MeshInfo>& MESHES, const AnimationData& ANIM_DATA);
		void InitMeshBuffers(ID3D11Device* pDevice, const MeshInfo& MESH_DATA, Mesh* pNewMesh) override;
		void InitAnimationData(ID3D11Device* pDevice, const AnimationData& ANIM_DATA);

		void UpdateAnimation(ID3D11DeviceContext* pContext, int clipID, int frame) override;

		void Render(ID3D11DeviceContext* pContext) override;

		inline Graphics::GraphicsPSO& GetPSO(const bool bWIRED) override { return (bWIRED ? Graphics::g_SkinnedWirePSO : Graphics::g_SkinnedSolidPSO); }
		inline Graphics::GraphicsPSO& GetDepthOnlyPSO() override { return Graphics::g_DepthOnlySkinnedPSO; }
		inline Graphics::GraphicsPSO& GetReflectPSO(const bool bWIRED) override { return (bWIRED ? Graphics::g_ReflectSkinnedWirePSO : Graphics::g_ReflectSkinnedSolidPSO); }

	public:
		Core::StructuredBuffer<Matrix> m_BoneTransforms;
		AnimationData m_AnimData;
	};
}
