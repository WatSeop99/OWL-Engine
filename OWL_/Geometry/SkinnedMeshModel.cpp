#include "../Common.h"
#include "../Renderer/BaseRenderer.h"
#include "../Graphics/GraphicsUtils.h"
#include "Mesh.h"
#include "../Renderer/StructuredBuffer.h"
#include "../Renderer/ResourceManager.h"
#include "SkinnedMeshModel.h"

void SkinnedMeshModel::Initialize(BaseRenderer* pRenderer, const std::vector<MeshInfo>& MESHES, const AnimationData& ANIM_DATA)
{
	Model::Initialize(pRenderer, MESHES);
	InitAnimationData(ANIM_DATA);
}

void SkinnedMeshModel::InitMeshBuffers(const MeshInfo& MESH_DATA, Mesh* pNewMesh)
{
	_ASSERT(m_pRenderer);
	_ASSERT(pNewMesh);

	HRESULT hr = S_OK;
	ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	ID3D11Device* pDevice = m_pRenderer->GetDevice();

	hr = pResourceManager->CreateVertexBuffer(sizeof(SkinnedVertex), (UINT)MESH_DATA.SkinnedVertices.size(), &pNewMesh->pVertexBuffer, (void*)MESH_DATA.SkinnedVertices.data());
	BREAK_IF_FAILED(hr);

	hr = pResourceManager->CreateIndexBuffer(sizeof(UINT), (UINT)MESH_DATA.Indices.size(), &pNewMesh->pIndexBuffer, (void*)MESH_DATA.Indices.data());
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

	// 여기서는 AnimationClip이 SkinnedMesh라고 가정.
	// 일반적으로 모든 Animation이 SkinnedMesh Animation은 아님.

	ID3D11Device* pDevice = m_pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();
	
	const UINT64 TOTAL_BONE_COUNT = ANIM_DATA.BoneNameToID.size(); // 뼈의 수.
	std::vector<Matrix> initData(TOTAL_BONE_COUNT);

	if (!m_pBoneTransform)
	{
		m_pBoneTransform = new StructuredBuffer;
	}
	m_pBoneTransform->Initialize(pDevice, pContext, sizeof(Matrix), (UINT)TOTAL_BONE_COUNT, initData.data());
}

void SkinnedMeshModel::UpdateAnimation(const int CLIP_ID, const int FRAME)
{
	CharacterAnimaionData.Update(CLIP_ID, FRAME);

	Matrix* pBoneTransformData = (Matrix*)m_pBoneTransform->pSystemMem;
	for (UINT64 i = 0, size = CharacterAnimaionData.BoneIDToNames.size(); i < size; ++i)
	{
		pBoneTransformData[i] = CharacterAnimaionData.Get(CLIP_ID, (int)i, FRAME).Transpose();
	}
	m_pBoneTransform->Upload();
}

void SkinnedMeshModel::Render()
{
	_ASSERT(m_pRenderer);

	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	pContext->VSSetShaderResources(9, 1, &m_pBoneTransform->pSRV);

	Model::Render();

	ID3D11ShaderResourceView* pNullSRV = nullptr;
	pContext->VSSetShaderResources(9, 1, &pNullSRV);
}

void SkinnedMeshModel::Cleanup()
{
	if (m_pBoneTransform)
	{
		delete m_pBoneTransform;
		m_pBoneTransform = nullptr;
	}
}
