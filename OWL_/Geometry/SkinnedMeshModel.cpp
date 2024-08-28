#include "../Common.h"
#include "../Graphics/GraphicsCommon.h"
#include "../Graphics/GraphicsUtils.h"
#include "SkinnedMeshModel.h"

void SkinnedMeshModel::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<MeshInfo>& MESHES, const AnimationData& ANIM_DATA)
{
	InitAnimationData(pDevice, ANIM_DATA);
	Model::Initialize(pDevice, pContext, MESHES);
}

void SkinnedMeshModel::InitMeshBuffers(ID3D11Device* pDevice, const MeshInfo& MESH_DATA, Mesh* pNewMesh)
{
	_ASSERT(pDevice);
	_ASSERT(pNewMesh);

	HRESULT hr = S_OK;

	hr = CreateVertexBuffer(pDevice, MESH_DATA.SkinnedVertices, &(pNewMesh->pVertexBuffer));
	BREAK_IF_FAILED(hr);

	hr = CreateIndexBuffer(pDevice, MESH_DATA.Indices, &(pNewMesh->pIndexBuffer));
	BREAK_IF_FAILED(hr);

	pNewMesh->IndexCount = (UINT)MESH_DATA.Indices.size();
	pNewMesh->VertexCount = (UINT)MESH_DATA.SkinnedVertices.size();
	pNewMesh->Stride = sizeof(SkinnedVertex);
}

void SkinnedMeshModel::InitAnimationData(ID3D11Device* pDevice, const AnimationData& ANIM_DATA)
{
	_ASSERT(pDevice);

	if (ANIM_DATA.Clips.empty())
	{
		return;
	}

	CharacterAnimaionData = ANIM_DATA;

	// 여기서는 AnimationClip이 SkinnedMesh라고 가정.
	// 일반적으로 모든 Animation이 SkinnedMesh Animation은 아님.
	BoneTransforms.CPU.resize(ANIM_DATA.Clips[0].Keys.size()); // 뼈의 수.

	// 주의: 모든 Keys()의 갯수가 동일하지 않을 수 있음.
	for (UINT64 i = 0, size = ANIM_DATA.Clips[0].Keys.size(); i < size; ++i)
	{
		BoneTransforms.CPU[i] = Matrix();
	}
	BoneTransforms.Initialize(pDevice);
}

void SkinnedMeshModel::UpdateAnimation(ID3D11DeviceContext* pContext, const int CLIP_ID, const int FRAME)
{
	_ASSERT(pContext);

	CharacterAnimaionData.Update(CLIP_ID, FRAME);

	for (UINT64 i = 0, size = BoneTransforms.CPU.size(); i < size; ++i)
	{
		BoneTransforms.CPU[i] = CharacterAnimaionData.Get(CLIP_ID, i, FRAME).Transpose();
	}
	BoneTransforms.Upload(pContext);
}

void SkinnedMeshModel::Render(ID3D11DeviceContext* pContext)
{
	_ASSERT(pContext);

	// ConstantsBuffer 대신 StructuredBuffer 사용.
	pContext->VSSetShaderResources(9, 1, &(BoneTransforms.pSRV));

	// Skinned VS/PS는 GraphicsPSO를 통해 지정되므로, Model::Render() 같이 사용 가능.
	// 이때, 전용 매크로를 사용해줘야 함.
	Model::Render(pContext);
}