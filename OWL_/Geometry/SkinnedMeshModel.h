#pragma once

#include "Model.h"
#include "../Core/StructuredBuffer.h"


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

	inline GraphicsPSO& GetPSO(const bool bWIRED) override { return (bWIRED ? g_SkinnedWirePSO : g_SkinnedSolidPSO); }
	inline GraphicsPSO& GetGBufferPSO(const bool bWIRED) override { return (bWIRED ? g_GBufferSKinnedWirePSO : g_GBufferSkinnedPSO); }
	inline GraphicsPSO& GetDepthOnlyPSO() override { return g_DepthOnlySkinnedPSO; }
	inline GraphicsPSO& GetDepthOnlyCubePSO() override { return g_DepthOnlyCubeSkinnedPSO; }
	inline GraphicsPSO& GetDepthOnlyCascadePSO() override { return g_DepthOnlyCascadeSkinnedPSO; }
	inline GraphicsPSO& GetReflectPSO(const bool bWIRED) override { return (bWIRED ? g_ReflectSkinnedWirePSO : g_ReflectSkinnedSolidPSO); }

public:
	StructuredBuffer<Matrix> m_BoneTransforms;
	AnimationData m_AnimData;
};
