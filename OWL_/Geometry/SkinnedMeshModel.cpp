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

	// ���⼭�� AnimationClip�� SkinnedMesh��� ����.
	// �Ϲ������� ��� Animation�� SkinnedMesh Animation�� �ƴ�.
	BoneTransforms.CPU.resize(ANIM_DATA.Clips[0].Keys.size()); // ���� ��.

	// ����: ��� Keys()�� ������ �������� ���� �� ����.
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

	// ConstantsBuffer ��� StructuredBuffer ���.
	pContext->VSSetShaderResources(9, 1, &(BoneTransforms.pSRV));

	// Skinned VS/PS�� GraphicsPSO�� ���� �����ǹǷ�, Model::Render() ���� ��� ����.
	// �̶�, ���� ��ũ�θ� �������� ��.
	Model::Render(pContext);
}