//#pragma comment(lib, "libfbxsdk.lib")
//
//#include <fbxsdk.h>
//#include "../Common.h"
//#include "MeshInfo.h"
//#include "FBXLoader.h"
//
//HRESULT FBXLoader::Load(std::wstring& basePath, std::wstring& fileName)
//{
//    HRESULT hr = S_OK;
//
//    if (GetFileExtension(fileName).compare(L".gltf") == 0)
//    {
//        __debugbreak();
//        hr = E_FAIL;
//        goto LB_RET;
//    }
//
//    std::string fileNameA(fileName.begin(), fileName.end());
//    szBasePath = std::string(basePath.begin(), basePath.end());
//
//    fbxsdk::FbxManager* pSDKManager = nullptr;
//    fbxsdk::FbxImporter* pImporter = nullptr;
//    fbxsdk::FbxScene* pScene = nullptr;
//
//    if (!initializeSDKObjects(&pSDKManager, &pScene))
//    {
//        __debugbreak();
//    }
//
//    if (!initializeImporter(pSDKManager, fileNameA, &pImporter))
//    {
//        __debugbreak();
//    }
//
//    bool bLoaded = pImporter->Import(pScene);
//    if (bLoaded)
//    {
//        fbxsdk::FbxStatus status;
//        fbxsdk::FbxArray<fbxsdk::FbxString*> details;
//        fbxsdk::FbxSceneCheckUtility sceneCheck(pScene, &status, &details);
//        bool bNotify = (!sceneCheck.Validate(fbxsdk::FbxSceneCheckUtility::eCkeckData) && details.GetCount() > 0) || (pImporter->GetStatus().GetCode() != fbxsdk::FbxStatus::eSuccess);
//        if (bNotify)
//        {
//            __debugbreak();
//        }
//        
//        fbxsdk::FbxGlobalSettings& globalSetting = pScene->GetGlobalSettings();
//        fbxsdk::FbxAxisSystem sceneAxisSystem = globalSetting.GetAxisSystem();
//        fbxsdk::FbxAxisSystem ourAxisSystem(fbxsdk::FbxAxisSystem::eYAxis, fbxsdk::FbxAxisSystem::eParityOdd, fbxsdk::FbxAxisSystem::eLeftHanded);
//        if (sceneAxisSystem != ourAxisSystem)
//        {
//            ourAxisSystem.ConvertScene(pScene);
//        }
//
//        FbxSystemUnit sceneSystemUnit = globalSetting.GetSystemUnit();
//        if (sceneSystemUnit.GetScaleFactor() != 1.0f)
//        {
//            // The unit in this example is centimeter.
//            FbxSystemUnit::cm.ConvertScene(pScene);
//        }
//
//        fbxsdk::FbxGeometryConverter geomConverter(pSDKManager);
//        if (!geomConverter.Triangulate(pScene, true))
//        {
//            __debugbreak();
//        }
//
//        // 모든 메쉬에 대해, 정점에 영향 주는 뼈들의 목록을 생성.
//        // 트리 구조를 따라, 업데이트 순서대로 뼈들의 인덱스를 결정.
//        fbxsdk::FbxNode* pRootNode = nullptr;
//        for (int i = 0, size = pScene->GetRootNode()->GetChildCount(); i < size; ++i)
//        {
//            fbxsdk::FbxNode* pNode = pScene->GetRootNode()->GetChild(i);
//            if (pNode->GetChildCount() > 0)
//            {
//                pRootNode = pNode;
//                break;
//            }
//        }
//
//        int totalBoneCount = 0;
//        findDeformingBones(pRootNode, &totalBoneCount);
//
//        // 업데이트 순서대로 뼈 이름 저장. (BoneIDToNames)
//        AnimData.BoneIDToNames.resize(totalBoneCount);
//        for (auto iter = AnimData.BoneNameToID.begin(), endIter = AnimData.BoneNameToID.end(); iter != endIter; ++iter)
//        {
//            AnimData.BoneIDToNames[iter->second] = iter->first;
//        }
//
//        // 각 뼈마다 부모 인덱스를 저장할 준비.
//        AnimData.BoneParents.resize(totalBoneCount, -1);
//        AnimData.NodeTransforms.resize(totalBoneCount);
//        AnimData.OffsetMatrices.resize(totalBoneCount);
//        AnimData.InverseOffsetMatrices.resize(totalBoneCount);
//        AnimData.BoneTransforms.resize(totalBoneCount);
//
//        Matrix globalTransform; // Initial transformation.
//        processNode();
//
//        // 애니메이션 정보 읽기.
//        if (pSCENE->HasAnimations())
//        {
//            readAnimation(pSCENE);
//        }
//
//        updateTangents();
//
//        pScene->Destroy();
//    }
//    else
//    {
//        hr = E_FAIL;
//    }
//
//    pImporter->Destroy();
//    pSDKManager->Destroy();
//
//LB_RET:
//    return hr;
//}
//
//HRESULT FBXLoader::LoadAnimation(std::wstring& basePath, std::wstring& fileName)
//{
//    return E_NOTIMPL;
//}
//
//bool FBXLoader::initializeSDKObjects(fbxsdk::FbxManager** ppOutManager, fbxsdk::FbxScene** ppOutScene)
//{
//    _ASSERT(ppOutManager && !*ppOutManager);
//    _ASSERT(ppOutScene && !*ppOutScene);
//
//    *ppOutManager = fbxsdk::FbxManager::Create();
//    if (!*ppOutManager)
//    {
//        __debugbreak();
//        return false;
//    }
//
//    fbxsdk::FbxIOSettings* pIOS = fbxsdk::FbxIOSettings::Create(*ppOutManager, IOSROOT);
//    if (!pIOS)
//    {
//        __debugbreak();
//        return false;
//    }
//    (*ppOutManager)->SetIOSettings(pIOS);
//
//    *ppOutScene = fbxsdk::FbxScene::Create(*ppOutManager, "Scene");
//    if (!*ppOutScene)
//    {
//        __debugbreak();
//        return false;
//    }
//
//    return true;
//}
//
//bool FBXLoader::initializeImporter(fbxsdk::FbxManager* pSDKManager, std::string& fileName, fbxsdk::FbxImporter** ppOutImporter)
//{
//    _ASSERT(pSDKManager);
//    _ASSERT(ppOutImporter && !*ppOutImporter);
//    
//    *ppOutImporter = fbxsdk::FbxImporter::Create(pSDKManager, "Importer");
//    if (!*ppOutImporter)
//    {
//        __debugbreak();
//        return false;
//    }
//
//    int fileFormat = -1;
//    fbxsdk::FbxIOPluginRegistry* pIOPlugin = pSDKManager->GetIOPluginRegistry();
//    if (pIOPlugin && !pIOPlugin->DetectReaderFileFormat(fileName.c_str(), fileFormat))
//    {
//        fileFormat = pIOPlugin->FindReaderIDByDescription("FBX binary (*.fbx)");
//    }
//    else
//    {
//        __debugbreak();
//        return false;
//    }
//
//    if (!(*ppOutImporter)->Initialize(fileName.c_str(), fileFormat))
//    {
//        __debugbreak();
//        return false;
//    }
//
//    return true;
//}
//
//void FBXLoader::findDeformingBones(fbxsdk::FbxNode* pNode, int* pCount)
//{
//    _ASSERT(pCount);
//
//    if (!pNode)
//    {
//        return;
//    }
//
//    fbxsdk::FbxNodeAttribute* pAttribute = pNode->GetNodeAttribute();
//    if (pAttribute && pAttribute->GetAttributeType() == fbxsdk::FbxNodeAttribute::eSkeleton)
//    {
//        AnimData.BoneNameToID[pNode->GetName()] = *pCount;
//        *pCount += 1;
//    }
//
//    const int CHILD_COUNT = pNode->GetChildCount();
//    for (int i = 0; i < CHILD_COUNT; ++i)
//    {
//        findDeformingBones(pNode->GetChild(i), pCount);
//    }
//}
//
//void FBXLoader::processNode(fbxsdk::FbxNode* pNode, Matrix& transform)
//{
//    if (!pNode)
//    {
//        return;
//    }
//
//	fbxsdk::FbxNodeAttribute* pAttribute = pNode->GetNodeAttribute();
//	if (pAttribute)
//	{
//        MeshInfo meshInfo = {};
//
//		switch (pAttribute->GetAttributeType())
//		{
//			case fbxsdk::FbxNodeAttribute::eMesh:
//			{
//                processMesh(pNode->GetMesh(), &meshInfo);
//				break;
//			}
//
//			default:
//				break;
//		}
//	}
//
//    const int CHILD_COUNT = pNode->GetChildCount();
//    for (int i = 0; i < CHILD_COUNT; ++i)
//    {
//        processNode(pNode->GetChild(i));
//    }
//}
//
//void FBXLoader::processNodeForAnimation(aiNode* pNode, const aiScene * pSCENE)
//{}
//
//void FBXLoader::processMesh(fbxsdk::FbxMesh* pMesh, MeshInfo* pMeshInfo)
//{
//
//}
//
//void FBXLoader::processMeshForAnimation(aiMesh* pMesh, const aiScene* pSCENE)
//{}
//
//void FBXLoader::readAnimation(const aiScene* pScene)
//{}
//
//HRESULT FBXLoader::readTextureFileName(const aiScene* pScene, aiMaterial* pMaterial, aiTextureType type, std::wstring* pDst)
//{
//    return E_NOTIMPL;
//}
//
//void FBXLoader::updateTangents()
//{}
//
//void FBXLoader::calculateTangentBitangent(const Vertex& V1, const Vertex& V2, const Vertex& V3, DirectX::XMFLOAT3* pTangent, DirectX::XMFLOAT3* pBitangent)
//{}
