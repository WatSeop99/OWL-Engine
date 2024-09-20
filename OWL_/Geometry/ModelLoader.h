#pragma once

#include "Animation.h"

enum aiTextureType;
struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;
struct MeshInfo;

class ModelLoader
{
public:
	ModelLoader() = default;
	~ModelLoader() = default;

	HRESULT Load(std::wstring& basePath, std::wstring& fileName, bool _bRevertNormals);
	HRESULT LoadAnimation(std::wstring& basePath, std::wstring& fileName);

protected:
	void findDeformingBones(const aiScene* pScene);
	const aiNode* findParent(const aiNode* pNode);

	void processNode(aiNode* pNode, const aiScene* pScene, Matrix& transform);
	void processMesh(aiMesh* pMesh, const aiScene* pScene, MeshInfo* pMeshInfo);

	void readAnimation(const aiScene* pScene);
	HRESULT readTextureFileName(const aiScene* pScene, aiMaterial* pMaterial, aiTextureType type, std::wstring* pDst);

	void updateTangents();
	void updateBoneIDs(aiNode* pNode, int* pCounter);

	void calculateTangentBitangent(const Vertex& V1, const Vertex& V2, const Vertex& V3, DirectX::XMFLOAT3* pTangent, DirectX::XMFLOAT3* pBitangent);

public:
	std::string szBasePath;
	std::vector<MeshInfo> pMeshInfos;

	AnimationData AnimData;

	bool bIsGLTF = false; // gltf or fbx.
	bool bRevertNormals = false;
};