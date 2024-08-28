#pragma once

#include <assimp/material.h>
#include "Animation.h"
#include "MeshInfo.h"


class ModelLoader
{
public:
	ModelLoader() = default;
	~ModelLoader() { pMeshInfos.clear(); }

	HRESULT Load(std::wstring& basePath, std::wstring& fileName, bool bRevertNormals_);
	HRESULT LoadAnimation(std::wstring& basePath, std::wstring& fileName);

protected:
	void findDeformingBones(const struct aiScene* pSCENE);
	const struct aiNode* findParent(const struct aiNode* pNODE);

	void processNode(struct aiNode* pNode, const struct aiScene* pSCENE, Matrix& transform);
	void processMesh(struct aiMesh* pMesh, const struct aiScene* pSCENE, MeshInfo* pMeshInfo);

	void readAnimation(const struct aiScene* pSCENE);
	HRESULT readTextureFileName(const struct aiScene* pSCENE, aiMaterial* pMaterial, aiTextureType type, std::wstring* pDst);

	void updateTangents();
	void updateBoneIDs(struct aiNode* pNode, int* pCounter);

	void calculateTangentBitangent(const Vertex& V1, const Vertex& V2, const Vertex& V3, DirectX::XMFLOAT3* pTangent, DirectX::XMFLOAT3* pBitangent);

public:
	std::string szBasePath;
	std::vector<MeshInfo> pMeshInfos;

	AnimationData AnimData;

	bool bIsGLTF = false; // gltf or fbx.
	bool bRevertNormals = false;
};