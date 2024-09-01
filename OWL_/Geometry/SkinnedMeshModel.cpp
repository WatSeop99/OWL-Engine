#include "../Common.h"
#include "../Renderer/BaseRenderer.h"
#include "../Graphics/GraphicsCommon.h"
#include "../Graphics/GraphicsUtils.h"
#include "Mesh.h"
#include "SkinnedMeshModel.h"

void SkinnedMeshModel::Initialize(BaseRenderer* pRenderer, const std::vector<MeshInfo>& MESHES, const AnimationData& ANIM_DATA)
{
	_ASSERT(pRenderer);

	Model::Initialize(pRenderer, MESHES);
	InitAnimationData(ANIM_DATA);
}

void SkinnedMeshModel::InitMeshBuffers(const MeshInfo& MESH_DATA, Mesh* pNewMesh)
{
	_ASSERT(m_pRenderer);
	_ASSERT(pNewMesh);

	HRESULT hr = S_OK;
	ID3D11Device* pDevice = m_pRenderer->GetDevice();

	hr = CreateVertexBuffer(pDevice, MESH_DATA.SkinnedVertices, &pNewMesh->pVertexBuffer);
	BREAK_IF_FAILED(hr);

	hr = CreateIndexBuffer(pDevice, MESH_DATA.Indices, &pNewMesh->pIndexBuffer);
	BREAK_IF_FAILED(hr);

	pNewMesh->IndexCount = (UINT)MESH_DATA.Indices.size();
	pNewMesh->VertexCount = (UINT)MESH_DATA.SkinnedVertices.size();
	pNewMesh->Stride = sizeof(SkinnedVertex);
}

void SkinnedMeshModel::InitAnimationData(const AnimationData& ANIM_DATA)
{
	_ASSERT(m_pRenderer);

	if (ANIM_DATA.Clips.empty())
	{
		return;
	}

	CharacterAnimaionData = ANIM_DATA;

	// ���⼭�� AnimationClip�� SkinnedMesh��� ����.
	// �Ϲ������� ��� Animation�� SkinnedMesh Animation�� �ƴ�.

	ID3D11Device* pDevice = m_pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();
	
	const UINT64 TOTAL_BONE_COUNT = ANIM_DATA.BoneNameToID.size(); // ���� ��.
	std::vector<Matrix> initData(TOTAL_BONE_COUNT);
	BoneTransforms.Initialize(pDevice, pContext, sizeof(Matrix), TOTAL_BONE_COUNT, initData.data());
}

void SkinnedMeshModel::UpdateAnimation(const int CLIP_ID, const int FRAME)
{
	CharacterAnimaionData.Update(CLIP_ID, FRAME);

	Matrix* pBoneTransformData = (Matrix*)BoneTransforms.pSystemMem;
	for (UINT64 i = 0, size = CharacterAnimaionData.BoneIDToNames.size(); i < size; ++i)
	{
		pBoneTransformData[i] = CharacterAnimaionData.Get(CLIP_ID, i, FRAME).Transpose();
	}
	BoneTransforms.Upload();
}

void SkinnedMeshModel::Render()
{
	_ASSERT(m_pRenderer);

	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	// ConstantsBuffer ��� StructuredBuffer ���.
	pContext->VSSetShaderResources(9, 1, &(BoneTransforms.pSRV));

	// Skinned VS/PS�� GraphicsPSO�� ���� �����ǹǷ�, Model::Render() ���� ��� ����.
	// �̶�, ���� ��ũ�θ� �������� ��.
	Model::Render();
}