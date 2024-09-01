#include "../Common.h"
#include "../Graphics/GraphicsCommon.h"
#include "../Graphics/GraphicsUtils.h"
#include "Mesh.h"
#include "SkinnedMeshModel.h"

void SkinnedMeshModel::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<MeshInfo>& MESHES, const AnimationData& ANIM_DATA)
{
	InitAnimationData(pDevice, pContext, ANIM_DATA);
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

void SkinnedMeshModel::InitAnimationData(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const AnimationData& ANIM_DATA)
{
	_ASSERT(pDevice);
	_ASSERT(pContext);

	if (ANIM_DATA.Clips.empty())
	{
		return;
	}

	CharacterAnimaionData = ANIM_DATA;

	// ���⼭�� AnimationClip�� SkinnedMesh��� ����.
	// �Ϲ������� ��� Animation�� SkinnedMesh Animation�� �ƴ�.
	
	const UINT64 TOTAL_BONE_COUNT = ANIM_DATA.BoneNameToID.size(); // ���� ��.
	std::vector<Matrix> initData(TOTAL_BONE_COUNT);
	BoneTransforms.Initialize(pDevice, pContext, sizeof(Matrix), TOTAL_BONE_COUNT, initData.data());
}

void SkinnedMeshModel::UpdateAnimation(ID3D11DeviceContext* pContext, const int CLIP_ID, const int FRAME)
{
	_ASSERT(pContext);

	CharacterAnimaionData.Update(CLIP_ID, FRAME);

	Matrix* pBoneTransformData = (Matrix*)BoneTransforms.pSystemMem;
	for (UINT64 i = 0, size = CharacterAnimaionData.BoneIDToNames.size(); i < size; ++i)
	{
		pBoneTransformData[i] = CharacterAnimaionData.Get(CLIP_ID, i, FRAME).Transpose();
	}
	BoneTransforms.Upload();
}

void SkinnedMeshModel::Render(ID3D11DeviceContext* pContext)
{
	_ASSERT(pContext);

	// ConstantsBuffer ��� StructuredBuffer ���.
	pContext->VSSetShaderResources(9, 1, &(BoneTransforms.pSRV));

	// Skinned VS/PS�� GraphicsPSO�� ���� �����ǹǷ�, Model::Render() ���� ��� ����.
	// �̶�, ���� ��ũ�θ� �������� ��.
	Model::Render(pContext);
}