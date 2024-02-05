#pragma once

#include <assimp/material.h>
#include "Animation.h"

namespace Geometry
{
	using std::map;
	using std::string;
	using std::wstring;
	using std::vector;

	class ModelLoader
	{
	public:
		ModelLoader() : bIsGLTF(false), bRevertNormals(false) { }
		~ModelLoader() { pMeshes.clear(); }

		HRESULT Load(std::wstring& basePath, std::wstring& fileName, bool bRevertNormals_);
		HRESULT LoadAnimation(std::wstring& basePath, std::wstring& fileName);

	protected:
		void findDeformingBones(const struct aiScene* pSCENE);
		const struct aiNode* findParent(const struct aiNode* pNODE);

		void processNode(struct aiNode* pNode, const struct aiScene* pSCENE, Matrix& transform);
		void processMesh(struct aiMesh* pMesh, const struct aiScene* pSCENE, struct MeshData* pMeshData);

		void readAnimation(const struct aiScene* pSCENE);
		HRESULT readTextureFileName(const struct aiScene* pSCENE, struct aiMaterial* pMaterial, aiTextureType type, std::wstring* pDst);

		void updateTangents();
		void updateBoneIDs(struct aiNode* pNode, int* pCounter);

		void calculateTangentBitangent(const struct Vertex& V1, const struct Vertex& V2, const struct Vertex& V3,
									   DirectX::XMFLOAT3* pTangent, DirectX::XMFLOAT3* pBitangent);

	public:
		std::string szBasePath;
		std::vector<struct MeshData> pMeshes;

		AnimationData AnimData;

		bool bIsGLTF; // gltf or fbx.
		bool bRevertNormals;
	};
}