//#pragma once
//
//#include "Animation.h"
//
//enum aiTextureType;
//struct aiNode;
//struct aiScene;
//struct aiMesh;
//struct aiMaterial;
//struct MeshInfo;
//class FbxManager;
//class FbxScene;
//
//class FBXLoader
//{
//public:
//	FBXLoader() = default;
//	~FBXLoader() = default;
//
//	HRESULT Load(std::wstring& basePath, std::wstring& fileName);
//	HRESULT LoadAnimation(std::wstring& basePath, std::wstring& fileName);
//
//protected:
//	bool initializeSDKObjects(fbxsdk::FbxManager** ppOutManager, fbxsdk::FbxScene** ppOutScene);
//	bool initializeImporter(fbxsdk::FbxManager* pSDKManager, std::string& fileName, fbxsdk::FbxImporter** ppOutImporter);
//
//	void findDeformingBones(const fbxsdk::FbxNode* pNODE, int* pCount);
//	const aiNode* findParent(const aiNode* pNode);
//
//	void processNode(const fbxsdk::FbxNode* pNODE, Matrix& transform);
//	void processNodeForAnimation(aiNode* pNode, const aiScene* pSCENE);
//	void processMesh(aiMesh* pMesh, const aiScene* pScene, MeshInfo* pMeshInfo);
//	void processMeshForAnimation(aiMesh* pMesh, const aiScene* pSCENE);
//
//	void readAnimation(const aiScene* pScene);
//	HRESULT readTextureFileName(const aiScene* pScene, aiMaterial* pMaterial, aiTextureType type, std::wstring* pDst);
//
//	void updateTangents();
//
//	void calculateTangentBitangent(const Vertex& V1, const Vertex& V2, const Vertex& V3, DirectX::XMFLOAT3* pTangent, DirectX::XMFLOAT3* pBitangent);
//
//public:
//	std::string szBasePath;
//	std::vector<MeshInfo> MeshInfos;
//
//	AnimationData AnimData;
//};
//
