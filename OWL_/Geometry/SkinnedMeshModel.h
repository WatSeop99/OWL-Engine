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

	inline eGraphicsPSOType GetPSO(const bool bWIRED) override { return (bWIRED ? GraphicsPSOType_SkinnedWire : GraphicsPSOType_SkinnedSolid); }
	inline eGraphicsPSOType GetGBufferPSO(const bool bWIRED) override { return (bWIRED ? GraphicsPSOType_GBufferSkinnedWire : GraphicsPSOType_GBufferSkinned); }
	inline eGraphicsPSOType GetDepthOnlyPSO() override { return GraphicsPSOType_DepthOnlySkinned; }
	inline eGraphicsPSOType GetDepthOnlyCubePSO() override { return GraphicsPSOType_DepthOnlyCubeSkinned; }
	inline eGraphicsPSOType GetDepthOnlyCascadePSO() override { return GraphicsPSOType_DepthOnlyCascadeSkinned; }
	inline eGraphicsPSOType GetReflectPSO(const bool bWIRED) override { return (bWIRED ? GraphicsPSOType_ReflectSkinnedWire : GraphicsPSOType_ReflectSkinnedSolid); }

public:
	StructuredBuffer BoneTransforms;
	AnimationData CharacterAnimaionData;
};
