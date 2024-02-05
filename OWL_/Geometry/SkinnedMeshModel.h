#pragma once

#include "../Core/StructuredBuffer.h"

namespace Geometry
{
	class SkinnedMeshModel : public Model
	{
	public:
		SkinnedMeshModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<struct MeshData>& MESHES, const AnimationData& ANIM_DATA);
		~SkinnedMeshModel() = default;
		
		void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<struct MeshData>& MESHES, const AnimationData& ANIM_DATA);
		void InitMeshBuffers(ID3D11Device* pDevice, const struct MeshData& MESH_DATA, struct Mesh* pNewMesh) override;
		void InitAnimationData(ID3D11Device* pDevice, const AnimationData& ANIM_DATA);

		void UpdateAnimation(ID3D11DeviceContext* pContext, int clipID, int frame) override;

		Graphics::GraphicsPSO& GetPSO(const bool bWIRED) override;
		Graphics::GraphicsPSO& GetDepthOnlyPSO() override;
		Graphics::GraphicsPSO& GetReflectPSO(const bool bWIRED) override;
		
		void Render(ID3D11DeviceContext* pContext) override;

	public:
		Core::StructuredBuffer<Matrix> m_BoneTransforms;
		AnimationData m_AnimData;
	};
}
