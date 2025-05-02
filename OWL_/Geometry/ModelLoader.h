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
	void FindDeformingBones(const aiScene* pScene);
	const aiNode* FindParent(const aiNode* pNode);

	void ProcessNode(aiNode* pNode, const aiScene* pScene, Matrix& transform);
	void ProcessNodeForAnimation(aiNode* pNode, const aiScene* pSCENE);
	void PocessMesh(aiMesh* pMesh, const aiScene* pScene, MeshInfo* pMeshInfo);
	void ProcessMeshForAnimation(aiMesh* pMesh, const aiScene* pSCENE);

	void ReadAnimation(const aiScene* pScene);
	HRESULT ReadTextureFileName(const aiScene* pScene, aiMaterial* pMaterial, aiTextureType type, std::wstring* pDst);

	void UpdateTangents();
	void UpdateBoneIDs(aiNode* pNode, int* pCounter);

	void CalculateTangentBitangent(const Vertex& V1, const Vertex& V2, const Vertex& V3, DirectX::XMFLOAT3* pTangent, DirectX::XMFLOAT3* pBitangent);

public:
	std::string szBasePath;
	std::vector<MeshInfo> MeshInfos;

	AnimationData AnimData;

	bool bIsGLTF = false; // gltf or fbx.
	bool bRevertNormals = false;
};