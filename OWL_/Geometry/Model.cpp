#include "../Common.h"
#include "GeometryGenerator.h"
#include "../Core/GraphicsUtils.h"
#include "Model.h"

DirectX::BoundingBox GetBoundingBox(const std::vector<Geometry::Vertex>& VERTICES)
{
	using DirectX::SimpleMath::Vector3;

	if (VERTICES.size() == 0)
	{
		return DirectX::BoundingBox();
	}

	Vector3 minCorner = VERTICES[0].Position;
	Vector3 maxCorner = VERTICES[0].Position;

	for (size_t i = 1, size = VERTICES.size(); i < size; ++i)
	{
		minCorner = Vector3::Min(minCorner, VERTICES[i].Position);
		maxCorner = Vector3::Max(maxCorner, VERTICES[i].Position);
	}

	Vector3 center = (minCorner + maxCorner) * 0.5f;
	Vector3 extents = maxCorner - center;

	return DirectX::BoundingBox(center, extents);
}

void ExtendBoundingBox(const DirectX::BoundingBox& SRC_BOX, DirectX::BoundingBox* pDestBox)
{
	using DirectX::SimpleMath::Vector3;

	Vector3 minCorner = Vector3(SRC_BOX.Center) - Vector3(SRC_BOX.Extents);
	Vector3 maxCorner = Vector3(SRC_BOX.Center) - Vector3(SRC_BOX.Extents);

	minCorner = Vector3::Min(minCorner, Vector3(pDestBox->Center) - Vector3(pDestBox->Extents));
	maxCorner = Vector3::Max(maxCorner, Vector3(pDestBox->Center) + Vector3(pDestBox->Extents));

	pDestBox->Center = (minCorner + maxCorner) * 0.5f;
	pDestBox->Extents = maxCorner - pDestBox->Center;
}

namespace Geometry
{
	using namespace std;
	using namespace DirectX;

	Model::Model(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, std::wstring& basePath, std::wstring& fileName) :
		World(), WorldInverseTranspose(),
		bDrawNormals(false), bIsVisible(true), bCastShadow(true), bIsPickable(false),
		Name("NoName")
	{
		Initialize(pDevice, pContext, basePath, fileName);
	}

	Model::Model(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<MeshInfo>& MESH_INFOS) :
		World(), WorldInverseTranspose(),
		bDrawNormals(false), bIsVisible(true), bCastShadow(true), bIsPickable(false),
		Name("NoName")
	{
		Initialize(pDevice, pContext, MESH_INFOS);
	}

	void Model::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, std::wstring& basePath, std::wstring& fileName)
	{
		std::vector<MeshInfo> meshInfos;
		Geometry::ReadFromFile(meshInfos, basePath, fileName);
		Initialize(pDevice, pContext, meshInfos);
	}

	void Model::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const vector<MeshInfo>& MESH_INFOS)
	{
		// Mesh들이 m_mesh/materialConsts를 각자 소유.
		HRESULT hr = S_OK;
		struct _stat64 sourceFileStat;

		/*MeshConstants.CPU.World = Matrix();
		MeshConstants.Initialize(pDevice);
		MaterialConstants.Initialize(pDevice);*/

		pMeshes.reserve(MESH_INFOS.size());
		for (size_t i = 0, meshSize = MESH_INFOS.size(); i < meshSize; ++i)
		{
			const MeshInfo& MESH_DATA = MESH_INFOS[i];
			Mesh* newMesh = (Mesh*)Malloc(sizeof(Mesh));
			*newMesh = INIT_MESH;

			newMesh->pMaterialBuffer = (Material*)Malloc(sizeof(Material));
			*(newMesh->pMaterialBuffer) = INIT_MATERIAL;

			newMesh->MeshConstants.CPU.World = Matrix();
			newMesh->MeshConstants.Initialize(pDevice);
			newMesh->MaterialConstants.Initialize(pDevice);
			InitMeshBuffers(pDevice, MESH_DATA, newMesh);

			if (!MESH_DATA.szAlbedoTextureFileName.empty())
			{
				std::string albedoTextureA(MESH_DATA.szAlbedoTextureFileName.begin(), MESH_DATA.szAlbedoTextureFileName.end());

				if (_stat64(albedoTextureA.c_str(), &sourceFileStat) != -1)
				{
					if (!MESH_DATA.szOpacityTextureFileName.empty())
					{
						hr = Graphics::CreateTexture(pDevice, pContext, MESH_DATA.szAlbedoTextureFileName.c_str(), MESH_DATA.szOpacityTextureFileName.c_str(), false,
													 newMesh->pMaterialBuffer->Albedo.GetAddressOfTexture(), newMesh->pMaterialBuffer->Albedo.GetAddressOfSRV());
					}
					else
					{
						hr = Graphics::CreateTexture(pDevice, pContext, MESH_DATA.szAlbedoTextureFileName.c_str(), true,
													 newMesh->pMaterialBuffer->Albedo.GetAddressOfTexture(), newMesh->pMaterialBuffer->Albedo.GetAddressOfSRV());
					}
					BREAK_IF_FAILED(hr);
					// SET_DEBUG_INFO_TO_OBJECT(newMesh->pAlbedoTexture, "Model::newMesh->pAlbedoTexture");
					// SET_DEBUG_INFO_TO_OBJECT(newMesh->pAlbedoSRV, "Model::newMesh->pAlbedoSRV");

					// MaterialConstants.CPU.bUseAlbedoMap = TRUE;
					newMesh->MaterialConstants.CPU.bUseAlbedoMap = TRUE;
				}
				else
				{
					OutputDebugStringW(MESH_DATA.szAlbedoTextureFileName.c_str());
					OutputDebugStringA(" does not exists. Skip texture reading.\n");
				}
			}

			if (!MESH_DATA.szEmissiveTextureFileName.empty())
			{
				std::string emissiveTextureA(MESH_DATA.szEmissiveTextureFileName.begin(), MESH_DATA.szEmissiveTextureFileName.end());

				if (_stat64(emissiveTextureA.c_str(), &sourceFileStat) != -1)
				{
					hr = Graphics::CreateTexture(pDevice, pContext, MESH_DATA.szEmissiveTextureFileName.c_str(), true,
												 newMesh->pMaterialBuffer->Emissive.GetAddressOfTexture(), newMesh->pMaterialBuffer->Emissive.GetAddressOfSRV());
					BREAK_IF_FAILED(hr);
					// SET_DEBUG_INFO_TO_OBJECT(newMesh->pEmissiveTexture, "Model::newMesh->pEmissiveTexture");
					// SET_DEBUG_INFO_TO_OBJECT(newMesh->pEmissiveSRV, "Model::newMesh->pEmissiveSRV");

					// MaterialConstants.CPU.bUseEmissiveMap = TRUE;
					newMesh->MaterialConstants.CPU.bUseEmissiveMap = TRUE;
				}
				else
				{
					OutputDebugStringW(MESH_DATA.szEmissiveTextureFileName.c_str());
					OutputDebugStringA(" does not exists. Skip texture reading.\n");
				}
			}

			if (!MESH_DATA.szNormalTextureFileName.empty())
			{
				std::string normalTextureA(MESH_DATA.szNormalTextureFileName.begin(), MESH_DATA.szNormalTextureFileName.end());

				if (_stat64(normalTextureA.c_str(), &sourceFileStat) != -1)
				{
					hr = Graphics::CreateTexture(pDevice, pContext, MESH_DATA.szNormalTextureFileName.c_str(), false,
												 newMesh->pMaterialBuffer->Normal.GetAddressOfTexture(), newMesh->pMaterialBuffer->Normal.GetAddressOfSRV());
					BREAK_IF_FAILED(hr);
					// SET_DEBUG_INFO_TO_OBJECT(newMesh->pNormalTexture, "Model::newMesh->pNormalTexture");
					// SET_DEBUG_INFO_TO_OBJECT(newMesh->pNormalSRV, "Model::newMesh->pNormalSRV");
					
					// MaterialConstants.CPU.bUseNormalMap = TRUE;
					newMesh->MaterialConstants.CPU.bUseNormalMap = TRUE;
				}
				else
				{
					OutputDebugStringW(MESH_DATA.szNormalTextureFileName.c_str());
					OutputDebugStringA(" does not exists. Skip texture reading.\n");
				}
			}

			if (!MESH_DATA.szHeightTextureFileName.empty())
			{
				std::string heightTextureA(MESH_DATA.szHeightTextureFileName.begin(), MESH_DATA.szHeightTextureFileName.end());

				if (_stat64(heightTextureA.c_str(), &sourceFileStat) != -1)
				{
					hr = Graphics::CreateTexture(pDevice, pContext, MESH_DATA.szHeightTextureFileName.c_str(), false,
												 newMesh->pMaterialBuffer->Height.GetAddressOfTexture(), newMesh->pMaterialBuffer->Height.GetAddressOfSRV());
					BREAK_IF_FAILED(hr);
					// SET_DEBUG_INFO_TO_OBJECT(newMesh->pHeightTexture, "Model::newMesh->pHeightTexture");
					// SET_DEBUG_INFO_TO_OBJECT(newMesh->pHeightSRV, "Model::newMesh->pHeightSRV");
					
					// MeshConstants.CPU.bUseHeightMap = TRUE;
					newMesh->MeshConstants.CPU.bUseHeightMap = TRUE;
				}
				else
				{
					OutputDebugStringW(MESH_DATA.szHeightTextureFileName.c_str());
					OutputDebugStringA(" does not exists. Skip texture reading.\n");
				}
			}

			if (!MESH_DATA.szAOTextureFileName.empty())
			{
				std::string aoTextureA(MESH_DATA.szAOTextureFileName.begin(), MESH_DATA.szAOTextureFileName.end());

				if (_stat64(aoTextureA.c_str(), &sourceFileStat) != -1)
				{
					hr = Graphics::CreateTexture(pDevice, pContext, MESH_DATA.szAOTextureFileName.c_str(), false,
												 newMesh->pMaterialBuffer->AmbientOcclusion.GetAddressOfTexture(), newMesh->pMaterialBuffer->AmbientOcclusion.GetAddressOfSRV());
					BREAK_IF_FAILED(hr);
					// SET_DEBUG_INFO_TO_OBJECT(newMesh->pAOTexture, "Model::newMesh->pAOTexture");
					// SET_DEBUG_INFO_TO_OBJECT(newMesh->pAOSRV, "Model::newMesh->pAOSRV");
					
					// MaterialConstants.CPU.bUseAOMap = TRUE;
					newMesh->MaterialConstants.CPU.bUseAOMap = TRUE;
				}
				else
				{
					OutputDebugStringW(MESH_DATA.szAOTextureFileName.c_str());
					OutputDebugStringA(" does not exists. Skip texture reading.\n");
				}
			}

			if (!MESH_DATA.szMetallicTextureFileName.empty())
			{
				std::string metallicTextureA(MESH_DATA.szMetallicTextureFileName.begin(), MESH_DATA.szMetallicTextureFileName.end());

				if (_stat64(metallicTextureA.c_str(), &sourceFileStat) != -1)
				{
					hr = Graphics::CreateTexture(pDevice, pContext, MESH_DATA.szMetallicTextureFileName.c_str(), false, 
												 newMesh->pMaterialBuffer->Metallic.GetAddressOfTexture(), newMesh->pMaterialBuffer->Metallic.GetAddressOfSRV());
					BREAK_IF_FAILED(hr);
					// SET_DEBUG_INFO_TO_OBJECT(newMesh->pMetallicTexture, "Model::newMesh->pMetallicTexture");
					// SET_DEBUG_INFO_TO_OBJECT(newMesh->pMetallicSRV, "Model::newMesh->pMetallicSRV");

					newMesh->MaterialConstants.CPU.bUseMetallicMap = TRUE;
				}
				else
				{
					OutputDebugStringW(MESH_DATA.szMetallicTextureFileName.c_str());
					OutputDebugStringA(" does not exists. Skip texture reading.\n");
				}
			}

			if (!MESH_DATA.szRoughnessTextureFileName.empty())
			{
				std::string roughnessTextureA(MESH_DATA.szRoughnessTextureFileName.begin(), MESH_DATA.szRoughnessTextureFileName.end());

				if (_stat64(roughnessTextureA.c_str(), &sourceFileStat) != -1)
				{
					hr = Graphics::CreateTexture(pDevice, pContext, MESH_DATA.szRoughnessTextureFileName.c_str(), false, 
												 newMesh->pMaterialBuffer->Roughness.GetAddressOfTexture(), newMesh->pMaterialBuffer->Roughness.GetAddressOfSRV());
					BREAK_IF_FAILED(hr);
					// SET_DEBUG_INFO_TO_OBJECT(newMesh->pRoughnessTexture, "Model::newMesh->pRoughnessTexture");
					// SET_DEBUG_INFO_TO_OBJECT(newMesh->pRoughnessSRV, "Model::newMesh->pRoughnessSRV");

					newMesh->MaterialConstants.CPU.bUseRoughnessMap = TRUE;
				}
				else
				{
					OutputDebugStringW(MESH_DATA.szRoughnessTextureFileName.c_str());
					OutputDebugStringA(" does not exists. Skip texture reading.\n");
				}
			}

			/*newMesh->pMeshConstantsGPU = MeshConstants.pGPU;
			newMesh->pMaterialConstantsGPU = MaterialConstants.pGPU;*/
			/*newMesh->MeshConstants.Upload(pContext);
			newMesh->MaterialConstants.Upload(pContext);*/

			pMeshes.push_back(newMesh);
		}

		// Bounding box 초기화.
		{
			BoundingBox = GetBoundingBox(MESH_INFOS[0].Vertices);
			for (size_t i = 1, size = MESH_INFOS.size(); i < size; ++i)
			{
				DirectX::BoundingBox bb = GetBoundingBox(MESH_INFOS[0].Vertices);
				ExtendBoundingBox(bb, &BoundingBox);
			}
			MeshInfo meshData = INIT_MESH_INFO;
			Geometry::MakeWireBox(&meshData, BoundingBox.Center, Vector3(BoundingBox.Extents) + Vector3(1e-3f));
			m_pBoundingBoxMesh = (Mesh*)Malloc(sizeof(Mesh));
			*m_pBoundingBoxMesh = INIT_MESH;

			m_pBoundingBoxMesh->pMaterialBuffer = (Material*)Malloc(sizeof(Material));
			*(m_pBoundingBoxMesh->pMaterialBuffer) = INIT_MATERIAL;

			hr = Graphics::CreateVertexBuffer(pDevice, meshData.Vertices, &(m_pBoundingBoxMesh->pVertexBuffer));
			BREAK_IF_FAILED(hr);
			SET_DEBUG_INFO_TO_OBJECT(m_pBoundingBoxMesh->pVertexBuffer, "Model::m_pBoundingBoxMesh->pVertexBuffer");

			hr = Graphics::CreateIndexBuffer(pDevice, meshData.Indices, &(m_pBoundingBoxMesh->pIndexBuffer));
			BREAK_IF_FAILED(hr);
			SET_DEBUG_INFO_TO_OBJECT(m_pBoundingBoxMesh->pIndexBuffer, "Model::m_pBoundingBoxMesh->pIndexBuffer");

			m_pBoundingBoxMesh->IndexCount = (UINT)(meshData.Indices.size());
			m_pBoundingBoxMesh->VertexCount = (UINT)(meshData.Vertices.size());
			m_pBoundingBoxMesh->Stride = sizeof(Vertex);

			m_pBoundingBoxMesh->MeshConstants.Initialize(pDevice);
			m_pBoundingBoxMesh->MaterialConstants.Initialize(pDevice);

			/*m_pBoundingBoxMesh->pMeshConstantsGPU = MeshConstants.pGPU;
			m_pBoundingBoxMesh->pMaterialConstantsGPU = MaterialConstants.pGPU;*/
			/*m_pBoundingBoxMesh->MeshConstants.Upload(pContext);
			m_pBoundingBoxMesh->MaterialConstants.Upload(pContext);*/
		}

		// Bounding sphere 초기화.
		{
			float maxRadius = 0.0f;
			for (size_t i = 0, size = MESH_INFOS.size(); i < size; ++i)
			{
				const MeshInfo& curMesh = MESH_INFOS[i];
				for (size_t j = 0, vertSize = curMesh.Vertices.size(); j < vertSize; ++j)
				{
					const Vertex& v = curMesh.Vertices[j];
					maxRadius = std::max((Vector3(BoundingBox.Center) - v.Position).Length(), maxRadius);
				}
			}

			maxRadius += 1e-2f; // 살짝 크게 설정.
			BoundingSphere = DirectX::BoundingSphere(BoundingBox.Center, maxRadius);

			MeshInfo meshData = INIT_MESH_INFO;
			Geometry::MakeWireSphere(&meshData, BoundingSphere.Center, BoundingSphere.Radius);
			m_pBoundingSphereMesh = (Mesh*)Malloc(sizeof(Mesh));
			*m_pBoundingSphereMesh = INIT_MESH;

			m_pBoundingSphereMesh->pMaterialBuffer = (Material*)Malloc(sizeof(Material));
			*(m_pBoundingSphereMesh->pMaterialBuffer) = INIT_MATERIAL;

			hr = Graphics::CreateVertexBuffer(pDevice, meshData.Vertices, &(m_pBoundingSphereMesh->pVertexBuffer));
			BREAK_IF_FAILED(hr);
			SET_DEBUG_INFO_TO_OBJECT(m_pBoundingSphereMesh->pVertexBuffer, "Model::m_pBoundingSphereMesh->pVertexBuffer");

			hr = Graphics::CreateIndexBuffer(pDevice, meshData.Indices, &(m_pBoundingSphereMesh->pIndexBuffer));
			BREAK_IF_FAILED(hr);
			SET_DEBUG_INFO_TO_OBJECT(m_pBoundingSphereMesh->pIndexBuffer, "Model::m_pBoundingSphereMesh->pIndexBuffer");

			m_pBoundingSphereMesh->IndexCount = (UINT)(meshData.Indices.size());
			m_pBoundingSphereMesh->VertexCount = (UINT)(meshData.Vertices.size());
			m_pBoundingSphereMesh->Stride = sizeof(Vertex);

			m_pBoundingSphereMesh->MeshConstants.Initialize(pDevice);
			m_pBoundingSphereMesh->MaterialConstants.Initialize(pDevice);

			/*m_pBoundingSphereMesh->pMeshConstantsGPU = MeshConstants.pGPU;
			m_pBoundingSphereMesh->pMaterialConstantsGPU = MaterialConstants.pGPU;*/
			/*m_pBoundingSphereMesh->MeshConstants.Upload(pContext);
			m_pBoundingSphereMesh->MaterialConstants.Upload(pContext);*/
		}
	}

	void Model::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	{
		OutputDebugStringA("Model::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext) was not implemented.");
		__debugbreak();
	}

	void Model::InitMeshBuffers(ID3D11Device* pDevice, const MeshInfo& MESH_INFO, Mesh* pNewMesh)
	{
		HRESULT hr = S_OK;
		hr = Graphics::CreateVertexBuffer(pDevice, MESH_INFO.Vertices, &(pNewMesh->pVertexBuffer));
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(pNewMesh->pVertexBuffer, "Model::pNewMesh->pVertexBuffer");

		hr = Graphics::CreateIndexBuffer(pDevice, MESH_INFO.Indices, &(pNewMesh->pIndexBuffer));
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(pNewMesh->pIndexBuffer, "Model::pNewMesh->pIndexBuffer");

		pNewMesh->VertexCount = (UINT)(MESH_INFO.Vertices.size());
		pNewMesh->IndexCount = (UINT)(MESH_INFO.Indices.size());
		pNewMesh->Stride = sizeof(Vertex);
	}

	void Model::UpdateConstantBuffers(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	{
		/*if (bIsVisible)
		{
			MeshConstants.Upload(pContext);
			MaterialConstants.Upload(pContext);
		}*/

		if (bIsVisible == false)
		{
			return;
		}

		for (size_t i = 0, size = pMeshes.size(); i < size; ++i)
		{
			Mesh* pCurMesh = pMeshes[i];
			pCurMesh->MeshConstants.Upload(pContext);
			pCurMesh->MaterialConstants.Upload(pContext);
		}
	}

	void Model::UpdateWorld(const Matrix& WORLD)
	{
		World = WORLD;
		WorldInverseTranspose = WORLD;
		WorldInverseTranspose.Translation(Vector3(0.0f));
		WorldInverseTranspose = WorldInverseTranspose.Invert().Transpose();

		// bounding sphere 위치 업데이트.
		// 스케일까지 고려하고 싶다면 x, y, z 스케일 중 가장 큰 값으로 스케일.
		// 구(sphere)라서 회전은 고려할 필요 없음.
		BoundingSphere.Center = World.Translation();

		/*MeshConstants.CPU.World = WORLD.Transpose();
		MeshConstants.CPU.WorldInverseTranspose = WorldInverseTranspose.Transpose();
		MeshConstants.CPU.WorldInverse = MeshConstants.CPU.World.Invert();*/

		for (size_t i = 0, size = pMeshes.size(); i < size; ++i)
		{
			Mesh* pCurMesh = pMeshes[i];
			pCurMesh->MeshConstants.CPU.World = WORLD.Transpose();
			pCurMesh->MeshConstants.CPU.WorldInverseTranspose = WorldInverseTranspose.Transpose();
			pCurMesh->MeshConstants.CPU.WorldInverse = pCurMesh->MeshConstants.CPU.World.Invert();
		}
	}

	void Model::UpdateAnimation(ID3D11DeviceContext* pContext, int clipID, int frame)
	{
		// class SkinnedMeshModel에서 override.
		/*OutputDebugStringA("Model::UpdateAnimation(ID3D11DeviceContext* pContext, int clipID, int frame) was not implemented.");
		__debugbreak();*/
	}

	void Model::Render(ID3D11DeviceContext* pContext)
	{
		if (bIsVisible == false)
		{
			return;
		}

		_ASSERT(pContext != nullptr);

		for (size_t i = 0, size = pMeshes.size(); i < size; ++i)
		{
			Mesh* const pCurMesh = pMeshes[i];

			ID3D11Buffer* ppConstantBuffers[] = { pCurMesh->MeshConstants.pGPU, pCurMesh->MaterialConstants.pGPU };
			UINT numConstantBuffers = _countof(ppConstantBuffers);
			pContext->VSSetConstantBuffers(2, numConstantBuffers, ppConstantBuffers);
			pContext->VSSetShaderResources(0, 1, pCurMesh->pMaterialBuffer->Height.GetAddressOfSRV());

			// 물체 렌더링할 때 여러가지 텍스춰 사용. (t0 부터시작)
			ID3D11ShaderResourceView* ppSRVs[] =
			{
				pCurMesh->pMaterialBuffer->Albedo.GetSRV(),
				pCurMesh->pMaterialBuffer->Emissive.GetSRV(),
				pCurMesh->pMaterialBuffer->Normal.GetSRV(),
				pCurMesh->pMaterialBuffer->AmbientOcclusion.GetSRV(),
				pCurMesh->pMaterialBuffer->Metallic.GetSRV(),
				pCurMesh->pMaterialBuffer->Roughness.GetSRV(),
			};
			UINT numSRVs = _countof(ppSRVs);
			pContext->PSSetShaderResources(0, numSRVs, ppSRVs);
			pContext->PSSetConstantBuffers(2, numConstantBuffers, ppConstantBuffers);

			// 볼륨 렌더링.
			ID3D11ShaderResourceView** ppDensitySRV = pCurMesh->pMaterialBuffer->Density.GetAddressOfSRV();
			ID3D11ShaderResourceView** ppLightingSRV = pCurMesh->pMaterialBuffer->Lighting.GetAddressOfSRV();
			if (*ppDensitySRV != nullptr)
			{
				pContext->PSSetShaderResources(6, 1, ppDensitySRV);
			}
			if (*ppLightingSRV != nullptr)
			{
				pContext->PSSetShaderResources(7, 1, ppLightingSRV);
			}

			pContext->IASetVertexBuffers(0, 1, &(pCurMesh->pVertexBuffer), &(pCurMesh->Stride), &(pCurMesh->Offset));
			pContext->IASetIndexBuffer(pCurMesh->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			pContext->DrawIndexed(pCurMesh->IndexCount, 0, 0);

			// Release resources.
			ID3D11ShaderResourceView* ppNulls[3] = { nullptr, nullptr, nullptr };
			pContext->PSSetShaderResources(6, 3, ppNulls);
		}
	}

	void Model::RenderNormals(ID3D11DeviceContext* pContext)
	{
		_ASSERT(pContext != nullptr);

		for (size_t i = 0, size = pMeshes.size(); i < size; ++i)
		{
			Mesh* const pCurMesh = pMeshes[i];

			ID3D11Buffer* ppConstantsBuffers[] = { pCurMesh->MeshConstants.pGPU, pCurMesh->MaterialConstants.pGPU };
			UINT numConstantsBuffers = _countof(ppConstantsBuffers);
			pContext->GSSetConstantBuffers(2, numConstantsBuffers, ppConstantsBuffers);
			pContext->IASetVertexBuffers(0, 1, &(pCurMesh->pVertexBuffer), &(pCurMesh->Stride), &(pCurMesh->Offset));
			pContext->Draw(pCurMesh->VertexCount, 0);
		}
	}

	void Model::RenderWireBoundingBox(ID3D11DeviceContext* pContext)
	{
		_ASSERT(pContext != nullptr);

		ID3D11Buffer* ppConstantsBuffers[] = { m_pBoundingBoxMesh->MeshConstants.pGPU, m_pBoundingBoxMesh->MaterialConstants.pGPU };
		UINT numConstantsBuffers = _countof(ppConstantsBuffers);
		pContext->VSSetConstantBuffers(2, numConstantsBuffers, ppConstantsBuffers);
		pContext->IASetVertexBuffers(0, 1, &(m_pBoundingBoxMesh->pVertexBuffer), &(m_pBoundingBoxMesh->Stride), &(m_pBoundingBoxMesh->Offset));
		pContext->IASetIndexBuffer(m_pBoundingBoxMesh->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		pContext->DrawIndexed(m_pBoundingBoxMesh->IndexCount, 0, 0);
	}

	void Model::RenderWireBoundingSphere(ID3D11DeviceContext* pContext)
	{
		_ASSERT(pContext != nullptr);

		ID3D11Buffer* ppConstantsBuffers[] = { m_pBoundingSphereMesh->MeshConstants.pGPU, m_pBoundingSphereMesh->MaterialConstants.pGPU };
		UINT numConstantBuffers = _countof(ppConstantsBuffers);
		pContext->VSSetConstantBuffers(2, numConstantBuffers, ppConstantsBuffers);
		pContext->IASetVertexBuffers(0, 1, &(m_pBoundingSphereMesh->pVertexBuffer), &(m_pBoundingSphereMesh->Stride), &(m_pBoundingSphereMesh->Offset));
		pContext->IASetIndexBuffer(m_pBoundingSphereMesh->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		pContext->DrawIndexed(m_pBoundingSphereMesh->IndexCount, 0, 0);
	}

	void Model::destroy()
	{
		if (m_pBoundingSphereMesh)
		{
			ReleaseMesh(&m_pBoundingSphereMesh);
		}
		if (m_pBoundingBoxMesh)
		{
			ReleaseMesh(&m_pBoundingBoxMesh);
		}

		for (size_t i = 0, size = pMeshes.size(); i < size; ++i)
		{
			ReleaseMesh(&pMeshes[i]);
		}
		pMeshes.clear();
	}
}
