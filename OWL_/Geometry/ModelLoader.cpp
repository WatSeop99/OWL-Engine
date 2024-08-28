#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "../Common.h"
#include "ModelLoader.h"


using namespace std;
using namespace DirectX::SimpleMath;

void UpdateNormals(std::vector<MeshInfo>& meshes)
{
	// 노멀 벡터가 없는 경우를 대비하여 다시 계산
	// 한 위치에는 한 버텍스만 있어야 연결 관계를 찾을 수 있음

	// DirectXMesh의 ComputeNormals()과 비슷합니다.
	// https://github.com/microsoft/DirectXMesh/wiki/ComputeNormals

	for (auto& m : meshes)
	{
		std::vector<Vector3> normalsTemp(m.Vertices.size(), Vector3(0.0f));
		std::vector<float> weightsTemp(m.Vertices.size(), 0.0f);

		for (int i = 0; i < m.Indices.size(); i += 3)
		{

			int idx0 = m.Indices[i];
			int idx1 = m.Indices[i + 1];
			int idx2 = m.Indices[i + 2];

			auto v0 = m.Vertices[idx0];
			auto v1 = m.Vertices[idx1];
			auto v2 = m.Vertices[idx2];

			auto faceNormal =
				(v1.Position - v0.Position).Cross(v2.Position - v0.Position);

			normalsTemp[idx0] += faceNormal;
			normalsTemp[idx1] += faceNormal;
			normalsTemp[idx2] += faceNormal;
			weightsTemp[idx0] += 1.0f;
			weightsTemp[idx1] += 1.0f;
			weightsTemp[idx2] += 1.0f;
		}

		for (int i = 0; i < m.Vertices.size(); i++)
		{
			if (weightsTemp[i] > 0.0f)
			{
				m.Vertices[i].Normal = normalsTemp[i] / weightsTemp[i];
				m.Vertices[i].Normal.Normalize();
			}
		}
	}
}

/*
 * 여러개의 뼈들이 있고 트리 구조임
 * 그 중에서 Vertex에 영향을 주는 것들은 일부임
 * Vertex에 영향을 주는 뼈들과 부모들까지 포함해서
 * 트래버스 순서로 저장
 */
HRESULT ModelLoader::Load(std::wstring& basePath, std::wstring& fileName, bool bRevertNormals_)
{
	HRESULT hr = S_OK;

	if (GetFileExtension(fileName).compare(L".gltf") == 0)
	{
		bIsGLTF = true;
		bRevertNormals = bRevertNormals_;
	}

	std::string fileNameA(fileName.begin(), fileName.end());
	szBasePath = std::string(basePath.begin(), basePath.end()); // 텍스춰 읽어들일 때 필요.

	Assimp::Importer importer;
	// ReadFile()에서 경우에 따라서 여러가지 옵션들 설정 가능
	// aiProcess_JoinIdenticalVertices | aiProcess_PopulateArmatureData |
	// aiProcess_SplitByBoneCount |
	// aiProcess_Debone); // aiProcess_LimitBoneWeights
	const aiScene* pSCENE = importer.ReadFile(szBasePath + fileNameA, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

	if (pSCENE)
	{
		// 모든 메쉬에 대해, 정점에 영향 주는 뼈들의 목록을 생성.
		findDeformingBones(pSCENE);

		// 트리 구조를 따라, 업데이트 순서대로 뼈들의 인덱스를 결정.
		int counter = 0;
		updateBoneIDs(pSCENE->mRootNode, &counter);

		// 업데이트 순서대로 뼈 이름 저장. (pBoneIDToNames)
		size_t totalBoneIDs = AnimData.BoneNameToID.size();
		AnimData.pBoneIDToNames.resize(totalBoneIDs);
		for (auto iter = AnimData.BoneNameToID.begin(), endIter = AnimData.BoneNameToID.end(); iter != endIter; ++iter)
		{
			AnimData.pBoneIDToNames[iter->second] = iter->first;
		}

		// 각 뼈마다 부모 인덱스를 저장할 준비.
		AnimData.pBoneParents.resize(totalBoneIDs, -1);

		Matrix tr; // Initial transformation.
		processNode(pSCENE->mRootNode, pSCENE, tr);

		// 애니메이션 정보 읽기.
		if (pSCENE->HasAnimations())
		{
			readAnimation(pSCENE);
		}

		// UpdateNormals(this->meshes); // Vertex Normal을 직접 계산 (참고용)

		updateTangents();
	}
	else
	{
		const char* pErrorDescription = importer.GetErrorString();

		OutputDebugStringA("Failed to read file: ");
		OutputDebugStringA((szBasePath + fileNameA).c_str());
		OutputDebugStringA("\n");


		OutputDebugStringA("Assimp error: ");
		OutputDebugStringA(pErrorDescription);
		OutputDebugStringA("\n");

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

		OutputDebugStringA("Failed to read animation from file: ");
		OutputDebugStringA((szBasePath + fileNameA).c_str());
		OutputDebugStringA("\n");


		OutputDebugStringA("Assimp error: ");
		OutputDebugStringA(pErrorDescription);
		OutputDebugStringA("\n");

		hr = E_FAIL;
	}

	return hr;
}

void ModelLoader::findDeformingBones(const aiScene* pSCENE)
{
	for (UINT i = 0; i < pSCENE->mNumMeshes; ++i)
	{
		const aiMesh* pMESH = pSCENE->mMeshes[i];
		if (pMESH->HasBones())
		{
			for (UINT j = 0; j < pMESH->mNumBones; ++j)
			{
				const aiBone* pBONE = pMESH->mBones[j];

				// bone과 대응되는 node의 이름은 동일.
				// 뒤에서 node 이름으로 부모를 찾을 수 있음.
				// 주의: 뼈의 순서가 업데이트 순서는 아님.
				AnimData.BoneNameToID[pBONE->mName.C_Str()] = -1;
			}
		}
	}
}

const aiNode* ModelLoader::findParent(const aiNode* pNODE)
{
	if (!pNODE)
	{
		return nullptr;
	}
	if (AnimData.BoneNameToID.count(pNODE->mName.C_Str()) > 0)
	{
		return pNODE;
	}
	return findParent(pNODE->mParent);
}

void ModelLoader::processNode(aiNode* pNode, const aiScene* pSCENE, Matrix& transform)
{
	// https://ogldev.org/www/tutorial38/tutorial38.html
	// If a node represents a bone in the hierarchy then the node name must
	// match the bone name.

	// 사용되는 부모 뼈를 찾아서 부모의 인덱스 저장.
	const aiNode* pPARENT = findParent(pNode->mParent);
	if (pNode->mParent &&
		AnimData.BoneNameToID.count(pNode->mName.C_Str()) > 0 &&
		pPARENT)
	{
		const int32_t BONE_ID = AnimData.BoneNameToID[pNode->mName.C_Str()];
		AnimData.pBoneParents[BONE_ID] = AnimData.BoneNameToID[pPARENT->mName.C_Str()];
	}

	Matrix m(&pNode->mTransformation.a1);
	m = m.Transpose() * transform;

	for (UINT i = 0; i < pNode->mNumMeshes; ++i)
	{
		aiMesh* pMesh = pSCENE->mMeshes[pNode->mMeshes[i]];
		MeshInfo newMeshInfo;

		processMesh(pMesh, pSCENE, &newMeshInfo);
		for (size_t j = 0, size = newMeshInfo.Vertices.size(); j < size; ++j)
		{
			Vertex& v = newMeshInfo.Vertices[j];
			v.Position = DirectX::SimpleMath::Vector3::Transform(v.Position, m);
		}

		pMeshInfos.push_back(newMeshInfo);
	}

	for (UINT i = 0; i < pNode->mNumChildren; ++i)
	{
		processNode(pNode->mChildren[i], pSCENE, m);
	}
}

void ModelLoader::processMesh(aiMesh* pMesh, const aiScene* pSCENE, MeshInfo* pMeshInfo)
{
	std::vector<Vertex>& vertices = pMeshInfo->Vertices;
	std::vector<uint32_t>& indices = pMeshInfo->Indices;
	std::vector<SkinnedVertex>& skinnedVertices = pMeshInfo->SkinnedVertices;

	// Walk through each of the mesh's vertices.
	vertices.resize(pMesh->mNumVertices);
	for (UINT i = 0; i < pMesh->mNumVertices; ++i)
	{
		Vertex& vertex = vertices[i];

		vertex.Position.x = pMesh->mVertices[i].x;
		vertex.Position.y = pMesh->mVertices[i].y;
		vertex.Position.z = pMesh->mVertices[i].z;

		vertex.Normal.x = pMesh->mNormals[i].x;
		if (bIsGLTF)
		{
			vertex.Normal.y = pMesh->mNormals[i].z;
			vertex.Normal.z = -pMesh->mNormals[i].y;
		}
		else
		{
			vertex.Normal.y = pMesh->mNormals[i].y;
			vertex.Normal.z = pMesh->mNormals[i].z;
		}

		if (bRevertNormals)
		{
			vertex.Normal *= -1.0f;
		}

		vertex.Normal.Normalize();

		if (pMesh->mTextureCoords[0])
		{
			vertex.Texcoord.x = (float)pMesh->mTextureCoords[0][i].x;
			vertex.Texcoord.y = (float)pMesh->mTextureCoords[0][i].y;
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

	if (pMesh->HasBones())
	{
		const size_t VERT_SIZE = vertices.size();
		std::vector<std::vector<float>> boneWeights(VERT_SIZE);
		std::vector<std::vector<uint8_t>> boneIndices(VERT_SIZE);

		AnimData.pOffsetMatrices.resize(AnimData.BoneNameToID.size());
		AnimData.pBoneTransforms.resize(AnimData.BoneNameToID.size());

		int count = 0;
		for (uint32_t i = 0; i < pMesh->mNumBones; ++i)
		{
			const aiBone* pBONE = pMesh->mBones[i];
			const uint32_t BONE_ID = AnimData.BoneNameToID[pBONE->mName.C_Str()];

			AnimData.pOffsetMatrices[BONE_ID] = Matrix((float*)&pBONE->mOffsetMatrix).Transpose();

			// 이 뼈가 영향을 주는 정점 개수.
			for (uint32_t j = 0; j < pBONE->mNumWeights; ++j)
			{
				aiVertexWeight weight = pBONE->mWeights[j];
				_ASSERT(weight.mVertexId < boneIndices.size());

				boneIndices[weight.mVertexId].push_back(BONE_ID);
				boneWeights[weight.mVertexId].push_back(weight.mWeight);
			}
		}

		// 예전에는 정점 하나에 영향을 주는 Bone은 최대 4개였음.
		// 요즘은 더 많을 수도 있는데 모델링 소프트웨어에서 조정하거나
		// 읽어들이면서 weight가 너무 작은 것들은 뺄 수도 있음.
		int maxBones = 0;
		for (size_t i = 0, boneWeightSize = boneWeights.size(); i < boneWeightSize; ++i)
		{
			maxBones = std::max(maxBones, (int)(boneWeights[i].size()));
		}

		char debugString[256];
		OutputDebugStringA("Max number of influencing bones per vertex = ");
		sprintf(debugString, "%d", maxBones);
		OutputDebugStringA(debugString);
		OutputDebugStringA("\n");

		skinnedVertices.resize(VERT_SIZE);
		for (size_t i = 0; i < VERT_SIZE; ++i)
		{
			skinnedVertices[i].Position = vertices[i].Position;
			skinnedVertices[i].Normal = vertices[i].Normal;
			skinnedVertices[i].Texcoord = vertices[i].Texcoord;

			for (size_t j = 0, curBoneWeightsSize = boneWeights[i].size(); j < curBoneWeightsSize; ++j)
			{
				skinnedVertices[i].BlendWeights[j] = boneWeights[i][j];
				skinnedVertices[i].BoneIndices[j] = boneIndices[i][j];
			}
		}
	}

	// http://assimp.sourceforge.net/lib_html/materials.html.
	if (pMesh->mMaterialIndex >= 0)
	{
		HRESULT hr = S_OK;
		aiMaterial* material = pSCENE->mMaterials[pMesh->mMaterialIndex];

		hr = readTextureFileName(pSCENE, material, aiTextureType_BASE_COLOR, &(pMeshInfo->szAlbedoTextureFileName));
		if (pMeshInfo->szAlbedoTextureFileName.empty())
		{
			hr = readTextureFileName(pSCENE, material, aiTextureType_DIFFUSE, &(pMeshInfo->szAlbedoTextureFileName));
		}
		hr = readTextureFileName(pSCENE, material, aiTextureType_EMISSIVE, &(pMeshInfo->szEmissiveTextureFileName));
		hr = readTextureFileName(pSCENE, material, aiTextureType_HEIGHT, &(pMeshInfo->szHeightTextureFileName));
		hr = readTextureFileName(pSCENE, material, aiTextureType_NORMALS, &(pMeshInfo->szNormalTextureFileName));
		hr = readTextureFileName(pSCENE, material, aiTextureType_METALNESS, &(pMeshInfo->szMetallicTextureFileName));
		hr = readTextureFileName(pSCENE, material, aiTextureType_DIFFUSE_ROUGHNESS, &(pMeshInfo->szRoughnessTextureFileName));
		hr = readTextureFileName(pSCENE, material, aiTextureType_AMBIENT_OCCLUSION, &(pMeshInfo->szAOTextureFileName));
		if (pMeshInfo->szAOTextureFileName.empty())
		{
			hr = readTextureFileName(pSCENE, material, aiTextureType_LIGHTMAP, &(pMeshInfo->szAOTextureFileName));
		}
		hr = readTextureFileName(pSCENE, material, aiTextureType_OPACITY, &(pMeshInfo->szOpacityTextureFileName)); // 불투명도를 표현하는 텍스쳐.

		if (!pMeshInfo->szOpacityTextureFileName.empty())
		{
			OutputDebugStringW(pMeshInfo->szAlbedoTextureFileName.c_str());
			OutputDebugStringA("\n");
			OutputDebugStringA("Opacity ");
			OutputDebugStringW(pMeshInfo->szOpacityTextureFileName.c_str());
			OutputDebugStringA("\n");
		}
	}
}

void ModelLoader::readAnimation(const aiScene* pSCENE)
{
	AnimData.pClips.resize(pSCENE->mNumAnimations);

	for (UINT i = 0; i < pSCENE->mNumAnimations; ++i)
	{
		AnimationClip& clip = AnimData.pClips[i];
		const aiAnimation* pANIM = pSCENE->mAnimations[i];

		clip.Duration = pANIM->mDuration;
		clip.TicksPerSec = pANIM->mTicksPerSecond;
		clip.pKeys.resize(AnimData.BoneNameToID.size());
		clip.NumChannels = pANIM->mNumChannels;

		for (UINT c = 0; c < pANIM->mNumChannels; ++c)
		{
			// channel은 각 뼈들의 움직임이 channel로 제공됨을 의미.
			const aiNodeAnim* pNODE_ANIM = pANIM->mChannels[c];
			const int BONE_ID = AnimData.BoneNameToID[pNODE_ANIM->mNodeName.C_Str()];
			clip.pKeys[BONE_ID].resize(pNODE_ANIM->mNumPositionKeys);

			for (UINT k = 0; k < pNODE_ANIM->mNumPositionKeys; ++k)
			{
				const aiVector3D POS = pNODE_ANIM->mPositionKeys[k].mValue;
				const aiQuaternion ROTATION = pNODE_ANIM->mRotationKeys[k].mValue;
				const aiVector3D SCALE = pNODE_ANIM->mScalingKeys[k].mValue;

				AnimationClip::Key& key = clip.pKeys[BONE_ID][k];
				key.Position = { POS.x, POS.y, POS.z };
				key.Rotation = Quaternion(ROTATION.x, ROTATION.y, ROTATION.z, ROTATION.w);
				key.Scale = { SCALE.x, SCALE.y, SCALE.z };
			}
		}
	}
}

HRESULT ModelLoader::readTextureFileName(const aiScene* pSCENE, aiMaterial* pMaterial, aiTextureType type, std::wstring* pDst)
{
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
				if (string(pTEXTURE->achFormatHint).find("png") != std::string::npos)
				{
					std::ofstream fileSystem(fullPath.c_str(), std::ios::binary | std::ios::out);
					fileSystem.write((char*)pTEXTURE->pcData, pTEXTURE->mWidth);
					fileSystem.close();
					// 참고: compressed format일 경우 texture->mHeight가 0.
				}
			}
			else
			{
				OutputDebugStringA(fullPath.c_str());
				OutputDebugStringA(" doesn't exists. Return empty filename.\n");
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
	for (size_t i = 0, size = pMeshInfos.size(); i < size; ++i)
	{
		// 방법 1.
		/*MeshInfo& m = pMeshInfos[i];

		std::vector<XMFLOAT3> positions(m.vertices.size());
		std::vector<XMFLOAT3> normals(m.vertices.size());
		std::vector<XMFLOAT2> texcoords(m.vertices.size());
		std::vector<XMFLOAT3> tangents(m.vertices.size());
		std::vector<XMFLOAT3> bitangents(m.vertices.size());

		for (size_t j = 0, vertSize = m.vertices.size(); j < vertSize; ++j)
		{
			Vertex& v = m.vertices[j];
			positions[j] = v.Position;
			normals[i] = v.Normal;
			texcoords[j] = v.Texcoord;
		}

		ComputeTangentFrame(m.indices.data(), m.indices.size() / 3,
							positions.data(), normals.data(), texcoords.data(),
							m.vertices.size(), tangents.data(),
							bitangents.data());

		for (size_t j = 0, vertSize = m.vertices.size(); j < vertSize; ++j)
		{
			m.vertices[j].Tangent = tangents[j];
		}

		if (m.skinnedVertices.size() > 0)
		{
			for (size_t j = 0, skinnedVertSize = m.skinnedVertices.size(); j < skinnedVertSize; ++j)
			{
				m.skinnedVertices[j].Tangent = tangents[j];
			}
		}*/

		// 방법 2.
		MeshInfo& curMeshInfo = pMeshInfos[i];
		std::vector<Vertex>& curVertices = curMeshInfo.Vertices;
		std::vector<SkinnedVertex>& curSkinnedVertices = curMeshInfo.SkinnedVertices;
		std::vector<uint32_t>& curIndices = curMeshInfo.Indices;
		size_t numFaces = curIndices.size() / 3;

		DirectX::XMFLOAT3 tangent;
		DirectX::XMFLOAT3 bitangent;

		for (size_t j = 0; j < numFaces; ++j)
		{
			calculateTangentBitangent(curVertices[curIndices[j * 3]], curVertices[curIndices[j * 3 + 1]], curVertices[curIndices[j * 3 + 2]], &tangent, &bitangent);

			curVertices[curIndices[j * 3]].Tangent = tangent;
			curVertices[curIndices[j * 3 + 1]].Tangent = tangent;
			curVertices[curIndices[j * 3 + 2]].Tangent = tangent;

			if (curSkinnedVertices.empty() == false) // vertices와 skinned vertices가 같은 크기를 가지고 있다고 가정.
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
	static int s_ID = 0;
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
