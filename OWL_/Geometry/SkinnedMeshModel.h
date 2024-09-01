#pragma once

#include "Animation.h"
#include "Model.h"
#include "../Renderer/StructuredBuffer.h"

class BaseRenderer;
class Mesh;

class SkinnedMeshModel final : public Model
{
public:
	SkinnedMeshModel() = default;
	~SkinnedMeshModel() = default;

	void Initialize(BaseRenderer* pRenderer, const std::vector<MeshInfo>& MESHES, const AnimationData& ANIM_DATA);
	void InitMeshBuffers(const MeshInfo& MESH_DATA, Mesh* pNewMesh) override;
	void InitAnimationData(const AnimationData& ANIM_DATA);

	void UpdateAnimation(const int CLIP_ID, const int FRAME) override;

	void Render() override;

	inline GraphicsPSO& GetPSO(const bool bWIRED) override { return (bWIRED ? g_SkinnedWirePSO : g_SkinnedSolidPSO); }
	inline GraphicsPSO& GetGBufferPSO(const bool bWIRED) override { return (bWIRED ? g_GBufferSKinnedWirePSO : g_GBufferSkinnedPSO); }
	inline GraphicsPSO& GetDepthOnlyPSO() override { return g_DepthOnlySkinnedPSO; }
	inline GraphicsPSO& GetDepthOnlyCubePSO() override { return g_DepthOnlyCubeSkinnedPSO; }
	inline GraphicsPSO& GetDepthOnlyCascadePSO() override { return g_DepthOnlyCascadeSkinnedPSO; }
	inline GraphicsPSO& GetReflectPSO(const bool bWIRED) override { return (bWIRED ? g_ReflectSkinnedWirePSO : g_ReflectSkinnedSolidPSO); }

public:
	StructuredBuffer BoneTransforms;
	AnimationData CharacterAnimaionData;
};
