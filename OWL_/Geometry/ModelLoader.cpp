#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "../Common.h"
#include "MeshInfo.h"
#include "ModelLoader.h"

using namespace DirectX::SimpleMath;

void UpdateNormals(std::vector<MeshInfo>& meshInfos)
{
	// 노멀 벡터가 없는 경우를 대비하여 다시 계산
	// 한 위치에는 한 버텍스만 있어야 연결 관계를 찾을 수 있음

	// DirectXMesh의 ComputeNormals()과 비슷합니다.
	// https://github.com/microsoft/DirectXMesh/wiki/ComputeNormals

	for (UINT64 i = 0, endI = meshInfos.size(); i < endI; ++i)
	{
		MeshInfo& meshInfo = meshInfos[i];

		std::vector<Vector3> normalsTemp(meshInfo.Vertices.size(), Vector3(0.0f));
		std::vector<float> weightsTemp(meshInfo.Vertices.size(), 0.0f);

		for (UINT64 j = 0, endJ = meshInfo.Indices.size(); j < endJ; j += 3)
		{
			UINT idx0 = meshInfo.Indices[j];
			UINT idx1 = meshInfo.Indices[j + 1];
			UINT idx2 = meshInfo.Indices[j + 2];

			Vertex v0 = meshInfo.Vertices[idx0];
			Vertex v1 = meshInfo.Vertices[idx1];
			Vertex v2 = meshInfo.Vertices[idx2];

			Vector3 faceNormal = (v1.Position - v0.Position).Cross(v2.Position - v0.Position);

			normalsTemp[idx0] += faceNormal;
			normalsTemp[idx1] += faceNormal;
			normalsTemp[idx2] += faceNormal;
			weightsTemp[idx0] += 1.0f;
			weightsTemp[idx1] += 1.0f;
			weightsTemp[idx2] += 1.0f;
		}

		for (UINT64 j = 0, endJ = meshInfo.Vertices.size(); j < endJ; ++j)
		{
			if (weightsTemp[j] > 0.0f)
			{
				Vector3& normal = meshInfo.Vertices[j].Normal;
				normal = normalsTemp[j] / weightsTemp[j];
				normal.Normalize();
			}
		}
	}
}

HRESULT ModelLoader::Load(std::wstring& basePath, std::wstring& fileName, bool _bRevertNormals)
{
	HRESULT hr = S_OK;

	if (GetFileExtension(fileName).compare(L".gltf") == 0)
	{
		bIsGLTF = true;
		bRevertNormals = _bRevertNormals;
	}

	std::string fileNameA(fileName.begin(), fileName.end());
	szBasePath = std::string(basePath.begin(), basePath.end());

	Assimp::Importer importer;
	// ReadFile()에서 경우에 따라서 여러가지 옵션들 설정 가능
	// aiProcess_JoinIdenticalVertices | aiProcess_PopulateArmatureData |
	// aiProcess_SplitByBoneCount |
	// aiProcess_Debone); // aiProcess_LimitBoneWeights
	const aiScene* pSCENE = importer.ReadFile(szBasePath + fileNameA, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_ConvertToLeftHanded);

	if (pSCENE)
	{
		// 모든 메쉬에 대해, 정점에 영향 주는 뼈들의 목록을 생성.
		findDeformingBones(pSCENE);

		// 트리 구조를 따라, 업데이트 순서대로 뼈들의 인덱스를 결정.
		int counter = 0;
		updateBoneIDs(pSCENE->mRootNode, &counter);

		// 업데이트 순서대로 뼈 이름 저장. (BoneIDToNames)
		size_t totalBoneIDs = AnimData.BoneNameToID.size();
		AnimData.BoneIDToNames.resize(totalBoneIDs);
		for (auto iter = AnimData.BoneNameToID.begin(), endIter = AnimData.BoneNameToID.end(); iter != endIter; ++iter)
		{
			AnimData.BoneIDToNames[iter->second] = iter->first;
		}

		// 각 뼈마다 부모 인덱스를 저장할 준비.
		AnimData.BoneParents.resize(totalBoneIDs, -1);

		Matrix tr; // Initial transformation.
		processNode(pSCENE->mRootNode, pSCENE, tr);

		// 애니메이션 정보 읽기.
		if (pSCENE->HasAnimations())
		{
			readAnimation(pSCENE);
		}

		// UpdateNormals(this->meshInfos); // Vertex Normal을 직접 계산 (참고용)

		updateTangents();
	}
	else
	{
		const char* pErrorDescription = importer.GetErrorString();
		char szDebugString[256];
		sprintf_s(szDebugString, 256, "Failed to read file: %s\nAssimp error: %s\n", (szBasePath + fileNameA).c_str(), pErrorDescription);
		OutputDebugStringA(szDebugString);

		hr = E_FAIL;
	}

	return hr;
}

HRESULT ModelLoader::LoadAnimation(std::wstring& basePath, std::wstring& fileName)
{
	HRESULT hr = S_OK;

	std::string fileNameA(fileName.begin(), fileName.end());
	szBasePath = std::string(basePath.begin(), basePath.end());

	Assimp::Importer importer;

	const aiScene* pSCENE = importer.ReadFile(szBasePath + fileNameA, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

	if (pSCENE && pSCENE->HasAnimations())
	{
		readAnimation(pSCENE);
	}
	else
	{
		const char* pErrorDescription = importer.GetErrorString();
		char szDebugString[256];
		sprintf_s(szDebugString, 256, "Failed to read animation from file: %s\n Assimp error: %s\n", (szBasePath + fileNameA).c_str(), pErrorDescription);
		OutputDebugStringA(szDebugString);

		hr = E_FAIL;
	}

	return hr;
}

void ModelLoader::findDeformingBones(const aiScene* pScene)
{
	_ASSERT(pScene);

	for (UINT i = 0; i < pScene->mNumMeshes; ++i)
	{
		const aiMesh* pMESH = pScene->mMeshes[i];
		if (pMESH->HasBones())
		{
			for (UINT j = 0; j < pMESH->mNumBones; ++j)
			{
				const aiBone* pBONE = pMESH->mBones[j];
				AnimData.BoneNameToID[pBONE->mName.C_Str()] = -1;
			}
		}
	}
}

const aiNode* ModelLoader::findParent(const aiNode* pNode)
{
	if (!pNode)
	{
		return nullptr;
	}
	if (AnimData.BoneNameToID.count(pNode->mName.C_Str()) > 0)
	{
		return pNode;
	}

	return findParent(pNode->mParent);
}

void ModelLoader::processNode(aiNode* pNode, const aiScene* pScene, Matrix& transform)
{
	// https://ogldev.org/www/tutorial38/tutorial38.html
	// If a node represents a bone in the hierarchy then the node name must
	// match the bone name.

	_ASSERT(pScene);

	if (!pNode)
	{
		return;
	}

	// 사용되는 부모 뼈를 찾아서 부모의 인덱스 저장.
	const aiNode* pPARENT = findParent(pNode->mParent);
	if (pNode->mParent &&
		AnimData.BoneNameToID.count(pNode->mName.C_Str()) > 0 &&
		pPARENT)
	{
		const int BONE_ID = AnimData.BoneNameToID[pNode->mName.C_Str()];
		AnimData.BoneParents[BONE_ID] = AnimData.BoneNameToID[pPARENT->mName.C_Str()];
	}

	Matrix m(&pNode->mTransformation.a1);
	m = m.Transpose() * transform;

	for (UINT i = 0; i < pNode->mNumMeshes; ++i)
	{
		aiMesh* pMesh = pScene->mMeshes[pNode->mMeshes[i]];
		MeshInfo newMeshInfo;

		processMesh(pMesh, pScene, &newMeshInfo);
		for (UINT64 j = 0, size = newMeshInfo.Vertices.size(); j < size; ++j)
		{
			Vertex& v = newMeshInfo.Vertices[j];
			v.Position = DirectX::SimpleMath::Vector3::Transform(v.Position, m);
		}

		pMeshInfos.push_back(newMeshInfo);
	}

	for (UINT i = 0; i < pNode->mNumChildren; ++i)
	{
		processNode(pNode->mChildren[i], pScene, m);
	}
}

void ModelLoader::processMesh(aiMesh* pMesh, const aiScene* pScene, MeshInfo* pMeshInfo)
{
	_ASSERT(pScene);
	_ASSERT(pMeshInfo);

	if (!pMesh)
	{
		return;
	}

	std::vector<Vertex>& vertices = pMeshInfo->Vertices;
	std::vector<UINT>& indices = pMeshInfo->Indices;
	std::vector<SkinnedVertex>& skinnedVertices = pMeshInfo->SkinnedVertices;

	// Walk through each of the mesh's vertices.
	vertices.resize(pMesh->mNumVertices);
	for (UINT i = 0; i < pMesh->mNumVertices; ++i)
	{
		Vertex& vertex = vertices[i];

		// position.
		if (pMesh->mVertices)
		{
			const aiVector3D& VERTEX = pMesh->mVertices[i];
			vertex.Position = { VERTEX.x, VERTEX.y, VERTEX.z };
		}

		// normal.
		if (pMesh->mNormals)
		{
			const aiVector3D& NORMAL = pMesh->mNormals[i];
			vertex.Normal.x = NORMAL.x;
			if (bIsGLTF)
			{
				vertex.Normal.y = NORMAL.z;
				vertex.Normal.z = -NORMAL.y;
			}
			else
			{
				vertex.Normal.y = NORMAL.y;
				vertex.Normal.z = NORMAL.z;
			}

			if (bRevertNormals)
			{
				vertex.Normal *= -1.0f;
			}

			vertex.Normal.Normalize();
		}
		
		// texcoord.
		if (pMesh->mTextureCoords[0])
		{
			const aiVector3D& TEXCOORD = pMesh->mTextureCoords[0][i];
			vertex.Texcoord = { TEXCOORD.x, TEXCOORD.y };
		}
	}

	indices.reserve(pMesh->mNumFaces);
	for (UINT i = 0; i < pMesh->mNumFaces; ++i)
	{
		aiFace face = pMesh->mFaces[i];
		for (UINT j = 0; j < face.mNumIndices; ++j)
		{
			indices.push_back(face.mIndices[j]);
		}
	}


	// http://assimp.sourceforge.net/lib_html/materials.html.
	if (pMesh->mMaterialIndex >= 0)
	{
		aiMaterial* pMaterial = pScene->mMaterials[pMesh->mMaterialIndex];

		readTextureFileName(pScene, pMaterial, aiTextureType_BASE_COLOR, &pMeshInfo->szAlbedoTextureFileName);
		if (pMeshInfo->szAlbedoTextureFileName.empty())
		{
			readTextureFileName(pScene, pMaterial, aiTextureType_DIFFUSE, &pMeshInfo->szAlbedoTextureFileName);
		}
		readTextureFileName(pScene, pMaterial, aiTextureType_EMISSIVE, &pMeshInfo->szEmissiveTextureFileName);
		readTextureFileName(pScene, pMaterial, aiTextureType_HEIGHT, &pMeshInfo->szHeightTextureFileName);
		readTextureFileName(pScene, pMaterial, aiTextureType_NORMALS, &pMeshInfo->szNormalTextureFileName);
		readTextureFileName(pScene, pMaterial, aiTextureType_METALNESS, &pMeshInfo->szMetallicTextureFileName);
		readTextureFileName(pScene, pMaterial, aiTextureType_DIFFUSE_ROUGHNESS, &pMeshInfo->szRoughnessTextureFileName);
		readTextureFileName(pScene, pMaterial, aiTextureType_AMBIENT_OCCLUSION, &pMeshInfo->szAOTextureFileName);
		if (pMeshInfo->szAOTextureFileName.empty())
		{
			readTextureFileName(pScene, pMaterial, aiTextureType_LIGHTMAP, &pMeshInfo->szAOTextureFileName);
		}
		readTextureFileName(pScene, pMaterial, aiTextureType_OPACITY, &pMeshInfo->szOpacityTextureFileName); // 불투명도를 표현하는 텍스쳐.

		if (!pMeshInfo->szOpacityTextureFileName.empty())
		{
			WCHAR szDebugString[256];
			swprintf_s(szDebugString, 256, L"%s\nOpacity %s\n", pMeshInfo->szAlbedoTextureFileName.c_str(), pMeshInfo->szOpacityTextureFileName.c_str());
			OutputDebugStringW(szDebugString);
		}
	}


	if (pMesh->HasBones())
	{
		const UINT64 VERT_SIZE = vertices.size();
		std::vector<std::vector<float>> boneWeights(VERT_SIZE);
		std::vector<std::vector<UINT8>> boneIndices(VERT_SIZE);

		AnimData.OffsetMatrices.resize(AnimData.BoneNameToID.size());
		AnimData.BoneTransforms.resize(AnimData.BoneNameToID.size());

		int count = 0;
		for (UINT i = 0; i < pMesh->mNumBones; ++i)
		{
			const aiBone* pBONE = pMesh->mBones[i];
			const int BONE_ID = AnimData.BoneNameToID[pBONE->mName.C_Str()];

			AnimData.OffsetMatrices[BONE_ID] = Matrix((float*)&pBONE->mOffsetMatrix).Transpose();

			// 이 뼈가 영향을 주는 정점 개수.
			for (UINT j = 0; j < pBONE->mNumWeights; ++j)
			{
				aiVertexWeight weight = pBONE->mWeights[j];
				_ASSERT(weight.mVertexId < boneIndices.size());

				boneIndices[weight.mVertexId].push_back(BONE_ID);
				boneWeights[weight.mVertexId].push_back(weight.mWeight);
			}
		}

#ifdef _DEBUG
		int maxBones = 0;
		for (UINT64 i = 0, boneWeightSize = boneWeights.size(); i < boneWeightSize; ++i)
		{
			maxBones = Max(maxBones, (int)(boneWeights[i].size()));
		}

		char debugString[256];
		sprintf_s(debugString, "Max number of influencing bones per vertex = %d\n", maxBones);
		OutputDebugStringA(debugString);
#endif

		skinnedVertices.resize(VERT_SIZE);
		for (UINT64 i = 0; i < VERT_SIZE; ++i)
		{
			skinnedVertices[i].Position = vertices[i].Position;
			skinnedVertices[i].Normal = vertices[i].Normal;
			skinnedVertices[i].Texcoord = vertices[i].Texcoord;

			for (UINT64 j = 0, curBoneWeightsSize = boneWeights[i].size(); j < curBoneWeightsSize; ++j)
			{
				skinnedVertices[i].BlendWeights[j] = boneWeights[i][j];
				skinnedVertices[i].BoneIndices[j] = boneIndices[i][j];
			}
		}
	}
}

void ModelLoader::readAnimation(const aiScene* pSCENE)
{
	_ASSERT(pSCENE);

	AnimData.Clips.resize(pSCENE->mNumAnimations);

	for (UINT i = 0; i < pSCENE->mNumAnimations; ++i)
	{
		AnimationClip& clip = AnimData.Clips[i];
		const aiAnimation* pANIM = pSCENE->mAnimations[i];

		clip.Duration = pANIM->mDuration;
		clip.TicksPerSec = pANIM->mTicksPerSecond;
		clip.Keys.resize(AnimData.BoneNameToID.size());
		clip.NumChannels = pANIM->mNumChannels;

		for (UINT c = 0; c < pANIM->mNumChannels; ++c)
		{
			// channel은 각 뼈들의 움직임이 channel로 제공됨을 의미.
			const aiNodeAnim* pNODE_ANIM = pANIM->mChannels[c];
			const int BONE_ID = AnimData.BoneNameToID[pNODE_ANIM->mNodeName.C_Str()];
			clip.Keys[BONE_ID].resize(pNODE_ANIM->mNumPositionKeys);

			for (UINT k = 0; k < pNODE_ANIM->mNumPositionKeys; ++k)
			{
				const aiVector3D POS = pNODE_ANIM->mPositionKeys[k].mValue;
				const aiQuaternion ROTATION = pNODE_ANIM->mRotationKeys[k].mValue;
				const aiVector3D SCALE = pNODE_ANIM->mScalingKeys[k].mValue;

				AnimationClip::Key& key = clip.Keys[BONE_ID][k];
				key.Position = { POS.x, POS.y, POS.z };
				key.Rotation = { ROTATION.x, ROTATION.y, ROTATION.z, ROTATION.w };
				key.Scale = { SCALE.x, SCALE.y, SCALE.z };
			}
		}
	}
}

HRESULT ModelLoader::readTextureFileName(const aiScene* pSCENE, aiMaterial* pMaterial, aiTextureType type, std::wstring* pDst)
{
	_ASSERT(pSCENE);
	_ASSERT(pMaterial);
	_ASSERT(pDst);

	HRESULT hr = S_OK;

	if (pMaterial->GetTextureCount(type) > 0)
	{
		aiString filePath;
		pMaterial->GetTexture(type, 0, &filePath);

		std::string fullPath = szBasePath + RemoveBasePath(filePath.C_Str());
		struct _stat64 sourceFileStat;

		// 실제로 파일이 존재하는지 확인.
		if (_stat64(fullPath.c_str(), &sourceFileStat) == -1)
		{
			// 파일이 없을 경우 혹시 fbx 자체에 Embedded인지 확인.
			const aiTexture* pTEXTURE = pSCENE->GetEmbeddedTexture(filePath.C_Str());
			if (pTEXTURE)
			{
				// Embedded texture가 존재하고 png일 경우 저장.
				// Embedded texture는 fbx와 같은 모델 파일에 텍스쳐도 같이 들어있는 경우.
				if (std::string(pTEXTURE->achFormatHint).find("png") != std::string::npos)
				{
					std::ofstream fileSystem(fullPath.c_str(), std::ios::binary | std::ios::out);
					fileSystem.write((char*)pTEXTURE->pcData, pTEXTURE->mWidth);
					fileSystem.close();
				}
			}
			else
			{
				char szDebugString[256];
				sprintf_s(szDebugString, 256, "%s doesn't exists. Return empty filename.\n", fullPath.c_str());
				OutputDebugStringA(szDebugString);
			}
		}
		else
		{
			*pDst = std::wstring(fullPath.begin(), fullPath.end());
		}
	}
	else
	{
		hr = E_FAIL;
	}

	return hr;
}

void ModelLoader::updateTangents()
{
	using namespace DirectX;

	// https://github.com/microsoft/DirectXMesh/wiki/ComputeTangentFrame
	// 추후 시간 체크 해볼 것.
	for (UINT64 i = 0, size = pMeshInfos.size(); i < size; ++i)
	{
		// 방법 1.
		/*MeshInfo& m = pMeshInfos[i];

		std::vector<XMFLOAT3> positions(m.vertices.endI());
		std::vector<XMFLOAT3> normals(m.vertices.endI());
		std::vector<XMFLOAT2> texcoords(m.vertices.endI());
		std::vector<XMFLOAT3> tangents(m.vertices.endI());
		std::vector<XMFLOAT3> bitangents(m.vertices.endI());

		for (size_t j = 0, vertSize = m.vertices.endI(); j < vertSize; ++j)
		{
			Vertex& v = m.vertices[j];
			positions[j] = v.Position;
			normals[i] = v.Normal;
			texcoords[j] = v.Texcoord;
		}

		ComputeTangentFrame(m.indices.data(), m.indices.endI() / 3,
							positions.data(), normals.data(), texcoords.data(),
							m.vertices.endI(), tangents.data(),
							bitangents.data());

		for (size_t j = 0, vertSize = m.vertices.endI(); j < vertSize; ++j)
		{
			m.vertices[j].Tangent = tangents[j];
		}

		if (m.skinnedVertices.endI() > 0)
		{
			for (size_t j = 0, skinnedVertSize = m.skinnedVertices.endI(); j < skinnedVertSize; ++j)
			{
				m.skinnedVertices[j].Tangent = tangents[j];
			}
		}*/

		// 방법 2.
		MeshInfo& curMeshInfo = pMeshInfos[i];
		std::vector<Vertex>& curVertices = curMeshInfo.Vertices;
		std::vector<SkinnedVertex>& curSkinnedVertices = curMeshInfo.SkinnedVertices;
		std::vector<UINT>& curIndices = curMeshInfo.Indices;
		UINT64 numFaces = curIndices.size() / 3;

		DirectX::XMFLOAT3 tangent;
		DirectX::XMFLOAT3 bitangent;

		for (UINT64 j = 0; j < numFaces; ++j)
		{
			calculateTangentBitangent(curVertices[curIndices[j * 3]], curVertices[curIndices[j * 3 + 1]], curVertices[curIndices[j * 3 + 2]], &tangent, &bitangent);

			curVertices[curIndices[j * 3]].Tangent = tangent;
			curVertices[curIndices[j * 3 + 1]].Tangent = tangent;
			curVertices[curIndices[j * 3 + 2]].Tangent = tangent;

			if (!curSkinnedVertices.empty()) // vertices와 skinned vertices가 같은 크기를 가지고 있다고 가정.
			{
				curSkinnedVertices[curIndices[j * 3]].Tangent = tangent;
				curSkinnedVertices[curIndices[j * 3 + 1]].Tangent = tangent;
				curSkinnedVertices[curIndices[j * 3 + 2]].Tangent = tangent;
			}
		}
	}
}

void ModelLoader::updateBoneIDs(aiNode* pNode, int* pCounter)
{
	_ASSERT(pCounter);

	if (pNode)
	{
		if (AnimData.BoneNameToID.count(pNode->mName.C_Str()))
		{
			AnimData.BoneNameToID[pNode->mName.C_Str()] = *pCounter;
			*pCounter += 1;
		}
		for (UINT i = 0; i < pNode->mNumChildren; ++i)
		{
			updateBoneIDs(pNode->mChildren[i], pCounter);
		}
	}
}

void ModelLoader::calculateTangentBitangent(const Vertex& V1, const Vertex& V2, const Vertex& V3, DirectX::XMFLOAT3* pTangent, DirectX::XMFLOAT3* pBitangent)
{
	DirectX::XMFLOAT3 vector1;
	DirectX::XMFLOAT3 vector2;
	DirectX::XMFLOAT2 tuVector;
	DirectX::XMFLOAT2 tvVector;
	DirectX::XMStoreFloat3(&vector1, V2.Position - V1.Position);
	DirectX::XMStoreFloat3(&vector2, V3.Position - V1.Position);
	DirectX::XMStoreFloat2(&tuVector, V2.Texcoord - V1.Texcoord);
	DirectX::XMStoreFloat2(&tvVector, V2.Texcoord - V3.Texcoord);

	float den = 1.0f / (tuVector.x * tvVector.y - tuVector.y * tvVector.x);

	pTangent->x = (tvVector.y * vector1.x - tvVector.x * vector2.x) * den;
	pTangent->y = (tvVector.y * vector1.y - tvVector.x * vector2.y) * den;
	pTangent->z = (tvVector.y * vector1.z - tvVector.x * vector2.z) * den;

	pBitangent->x = (tuVector.x * vector2.x - tuVector.y * vector1.x) * den;
	pBitangent->y = (tuVector.x * vector2.y - tuVector.y * vector1.y) * den;
	pBitangent->z = (tuVector.x * vector2.z - tuVector.y * vector1.z) * den;

	float length = sqrt((pTangent->x * pTangent->x) + (pTangent->y * pTangent->y) + (pTangent->z * pTangent->z));
	pTangent->x = pTangent->x / length;
	pTangent->y = pTangent->y / length;
	pTangent->z = pTangent->z / length;

	length = sqrt((pBitangent->x * pBitangent->x) + (pBitangent->y * pBitangent->y) + (pBitangent->z * pBitangent->z));
	pBitangent->x = pBitangent->x / length;
	pBitangent->y = pBitangent->y / length;
	pBitangent->z = pBitangent->z / length;
}
