#include "../Common.h"
#include "../Core/GraphicsCommon.h"
#include "../Core/GraphicsUtils.h"
#include "SkinnedMeshModel.h"

namespace Geometry
{
	SkinnedMeshModel::SkinnedMeshModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<MeshInfo>& MESHES, const AnimationData& ANIM_DATA)
	{
		Initialize(pDevice, pContext, MESHES, ANIM_DATA);
	}

	void SkinnedMeshModel::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<MeshInfo>& MESHES, const AnimationData& ANIM_DATA)
	{
		InitAnimationData(pDevice, ANIM_DATA);
		Model::Initialize(pDevice, pContext, MESHES);
	}

	void SkinnedMeshModel::InitMeshBuffers(ID3D11Device* pDevice, const MeshInfo& MESH_DATA, Mesh* pNewMesh)
	{
		HRESULT hr = S_OK;

		hr = Graphics::CreateVertexBuffer(pDevice, MESH_DATA.SkinnedVertices, &(pNewMesh->pVertexBuffer));
		BREAK_IF_FAILED(hr);

		hr = Graphics::CreateIndexBuffer(pDevice, MESH_DATA.Indices, &(pNewMesh->pIndexBuffer));
		BREAK_IF_FAILED(hr);

		pNewMesh->IndexCount = (UINT)(MESH_DATA.Indices.size());
		pNewMesh->VertexCount = (UINT)(MESH_DATA.SkinnedVertices.size());
		pNewMesh->Stride = sizeof(SkinnedVertex);
	}

	void SkinnedMeshModel::InitAnimationData(ID3D11Device* pDevice, const AnimationData& ANIM_DATA)
	{
		if (ANIM_DATA.pClips.empty() == false)
		{
			m_AnimData = ANIM_DATA;

			// 여기서는 AnimationClip이 SkinnedMesh라고 가정.
			// 일반적으로 모든 Animation이 SkinnedMesh Animation은 아님.
			m_BoneTransforms.CPU.resize(ANIM_DATA.pClips[0].pKeys.size()); // 뼈의 수.

			// 주의: 모든 pKeys()의 갯수가 동일하지 않을 수 있음.
			for (size_t i = 0, size = ANIM_DATA.pClips[0].pKeys.size(); i < size; ++i)
			{
				m_BoneTransforms.CPU[i] = Matrix();
			}
			m_BoneTransforms.Initialize(pDevice);
		}
	}

	void SkinnedMeshModel::UpdateAnimation(ID3D11DeviceContext* pContext, int clipID, int frame)
	{
		m_AnimData.Update(clipID, frame);

		for (size_t i = 0, size = m_BoneTransforms.CPU.size(); i < size; ++i)
		{
			m_BoneTransforms.CPU[i] = m_AnimData.Get(clipID, i, frame).Transpose();
		}
		m_BoneTransforms.Upload(pContext);
	}

	void SkinnedMeshModel::Render(ID3D11DeviceContext* pContext)
	{
		// ConstantsBuffer 대신 StructuredBuffer 사용.
		pContext->VSSetShaderResources(9, 1, &(m_BoneTransforms.pSRV));

		// Skinned VS/PS는 GraphicsPSO를 통해 지정되므로, Model::Render() 같이 사용 가능.
		// 이때, 전용 매크로를 사용해줘야 함.
		Model::Render(pContext);
	}
}
