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

			// ���⼭�� AnimationClip�� SkinnedMesh��� ����.
			// �Ϲ������� ��� Animation�� SkinnedMesh Animation�� �ƴ�.
			m_BoneTransforms.CPU.resize(ANIM_DATA.pClips[0].pKeys.size()); // ���� ��.

			// ����: ��� pKeys()�� ������ �������� ���� �� ����.
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
		// ConstantsBuffer ��� StructuredBuffer ���.
		pContext->VSSetShaderResources(9, 1, &(m_BoneTransforms.pSRV));

		// Skinned VS/PS�� GraphicsPSO�� ���� �����ǹǷ�, Model::Render() ���� ��� ����.
		// �̶�, ���� ��ũ�θ� �������� ��.
		Model::Render(pContext);
	}
}
