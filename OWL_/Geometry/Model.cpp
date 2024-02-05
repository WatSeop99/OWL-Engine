#include "../Common.h"
#include "GeometryGenerator.h"
#include "../Core/GraphicsCommon.h"
#include "../Core/GraphicsUtils.h"
#include "Mesh.h"
#include "MeshData.h"
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

	Model::Model(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<struct MeshData>& MESHES) :
		World(), WorldInverseTranspose(),
		bDrawNormals(false), bIsVisible(true), bCastShadow(true), bIsPickable(false),
		Name("NoName")
	{
		Initialize(pDevice, pContext, MESHES);
	}

	void Model::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, std::wstring& basePath, std::wstring& fileName)
	{
		std::vector<struct MeshData> meshes;
		Geometry::ReadFromFile(meshes, basePath, fileName);
		Initialize(pDevice, pContext, meshes);
	}

	void Model::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const vector<struct MeshData>& MESHES)
	{
		// 일반적으로는 Mesh들이 m_mesh/materialConsts를 각자 소유 가능.
		// 여기서는 한 Model 안의 여러 Mesh들이 Consts를 모두 공유.
		HRESULT hr = S_OK;

		MeshConstants.CPU.World = Matrix();
		MeshConstants.Initialize(pDevice);
		MaterialConstants.Initialize(pDevice);

		pMeshes.reserve(MESHES.size());
		for (size_t i = 0, meshSize = MESHES.size(); i < meshSize; ++i)
		{
			const struct MeshData& MESH_DATA = MESHES[i];
			struct Mesh* newMesh = (struct Mesh*)Malloc(sizeof(struct Mesh));
			struct _stat64 sourceFileStat;
			*newMesh = INIT_MESH;

			InitMeshBuffers(pDevice, MESH_DATA, newMesh);

			if (!MESH_DATA.szAlbedoTextureFileName.empty())
			{
				std::string albedoTextureA(MESH_DATA.szAlbedoTextureFileName.begin(), MESH_DATA.szAlbedoTextureFileName.end());

				if (_stat64(albedoTextureA.c_str(), &sourceFileStat) != -1)
				{
					if (!MESH_DATA.szOpacityTextureFileName.empty())
					{
						hr = Graphics::CreateTexture(pDevice, pContext, MESH_DATA.szAlbedoTextureFileName.c_str(), MESH_DATA.szOpacityTextureFileName.c_str(), false,
													 &(newMesh->pAlbedoTexture), &(newMesh->pAlbedoSRV));
					}
					else
					{
						hr = Graphics::CreateTexture(pDevice, pContext, MESH_DATA.szAlbedoTextureFileName.c_str(), true,
													 &(newMesh->pAlbedoTexture), &(newMesh->pAlbedoSRV));
					}
					BREAK_IF_FAILED(hr);
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pAlbedoTexture, "Model::newMesh->pAlbedoTexture");
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pAlbedoSRV, "Model::newMesh->pAlbedoSRV");

					MaterialConstants.CPU.bUseAlbedoMap = TRUE;
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
												 &(newMesh->pEmissiveTexture), &(newMesh->pEmissiveSRV));
					BREAK_IF_FAILED(hr);
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pEmissiveTexture, "Model::newMesh->pEmissiveTexture");
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pEmissiveSRV, "Model::newMesh->pEmissiveSRV");
					MaterialConstants.CPU.bUseEmissiveMap = TRUE;
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
												 &(newMesh->pNormalTexture), &(newMesh->pNormalSRV));
					BREAK_IF_FAILED(hr);
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pNormalTexture, "Model::newMesh->pNormalTexture");
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pNormalSRV, "Model::newMesh->pNormalSRV");
					MaterialConstants.CPU.bUseNormalMap = TRUE;
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
												 &(newMesh->pHeightTexture), &(newMesh->pHeightSRV));
					BREAK_IF_FAILED(hr);
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pHeightTexture, "Model::newMesh->pHeightTexture");
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pHeightSRV, "Model::newMesh->pHeightSRV");
					MeshConstants.CPU.bUseHeightMap = TRUE;
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
												 &(newMesh->pAOTexture), &(newMesh->pAOSRV));
					BREAK_IF_FAILED(hr);
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pAOTexture, "Model::newMesh->pAOTexture");
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pAOSRV, "Model::newMesh->pAOSRV");
					MaterialConstants.CPU.bUseAOMap = TRUE;
				}
				else
				{
					OutputDebugStringW(MESH_DATA.szAOTextureFileName.c_str());
					OutputDebugStringA(" does not exists. Skip texture reading.\n");
				}
			}

			// GLTF 방식으로 Metallic과 Roughness를 한 텍스춰에 넣음.
			// Green : Roughness, Blue : Metallic(Metalness).
			/*if (!MESH_DATA.szMetallicTextureFileName.empty() ||
				!MESH_DATA.szRoughnessTextureFileName.empty())
			{
				struct _stat64 sourceFileStat1;
				struct _stat64 sourceFileStat2;
				std::string metallicTextureA(MESH_DATA.szMetallicTextureFileName.begin(), MESH_DATA.szMetallicTextureFileName.end());
				std::string roughnessTextureA(MESH_DATA.szRoughnessTextureFileName.begin(), MESH_DATA.szRoughnessTextureFileName.end());

				if (_stat64(metallicTextureA.c_str(), &sourceFileStat1) != -1 &&
					_stat64(roughnessTextureA.c_str(), &sourceFileStat2) != -1)
				{

					hr = Graphics::CreateMetallicRoughnessTexture(pDevice, pContext, MESH_DATA.szMetallicTextureFileName.c_str(), MESH_DATA.szRoughnessTextureFileName.c_str(),
																  &(newMesh->pMetallicRoughnessTexture), &(newMesh->pMetallicRoughnessSRV));
					BREAK_IF_FAILED(hr);
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pMetallicRoughnessTexture, "Model::newMesh->pMetallicRoughnessTexture");
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pMetallicRoughnessSRV, "Model::newMesh->pMetallicRoughnessSRV");
				}
				else
				{
					OutputDebugStringW(MESH_DATA.szMetallicTextureFileName.c_str());
					OutputDebugStringA(" or ");
					OutputDebugStringW(MESH_DATA.szRoughnessTextureFileName.c_str());
					OutputDebugStringA(" does not exists. Skip texture reading.\n");
				}
			}
			if (!MESH_DATA.szMetallicTextureFileName.empty())
			{
				MaterialConstants.CPU.bUseMetallicMap = TRUE;
			}
			if (!MESH_DATA.szRoughnessTextureFileName.empty())
			{
				MaterialConstants.CPU.bUseRoughnessMap = TRUE;
			}*/

			if (!MESH_DATA.szMetallicTextureFileName.empty())
			{
				std::string metallicTextureA(MESH_DATA.szMetallicTextureFileName.begin(), MESH_DATA.szMetallicTextureFileName.end());

				if (_stat64(metallicTextureA.c_str(), &sourceFileStat) != -1)
				{
					/*hr = Graphics::CreateMetallicRoughnessTexture(pDevice, pContext, MESH_DATA.szMetallicTextureFileName.c_str(), MESH_DATA.szRoughnessTextureFileName.c_str(),
																  &(newMesh->pMetallicTexture), &(newMesh->pMetallicSRV));*/
					hr = Graphics::CreateTexture(pDevice, pContext, MESH_DATA.szMetallicTextureFileName.c_str(), false, 
												 &(newMesh->pMetallicTexture), &(newMesh->pMetallicSRV));
					BREAK_IF_FAILED(hr);
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pMetallicTexture, "Model::newMesh->pMetallicTexture");
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pMetallicSRV, "Model::newMesh->pMetallicSRV");
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
					/*hr = Graphics::CreateMetallicRoughnessTexture(pDevice, pContext, MESH_DATA.szMetallicTextureFileName.c_str(), MESH_DATA.szRoughnessTextureFileName.c_str(),
																  &(newMesh->pMetallicTexture), &(newMesh->pMetallicSRV));*/
					hr = Graphics::CreateTexture(pDevice, pContext, MESH_DATA.szRoughnessTextureFileName.c_str(), false, 
												 &(newMesh->pRoughnessTexture), &(newMesh->pRoughnessSRV));
					BREAK_IF_FAILED(hr);
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pRoughnessTexture, "Model::newMesh->pRoughnessTexture");
					SET_DEBUG_INFO_TO_OBJECT(newMesh->pRoughnessSRV, "Model::newMesh->pRoughnessSRV");
				}
				else
				{
					OutputDebugStringW(MESH_DATA.szRoughnessTextureFileName.c_str());
					OutputDebugStringA(" does not exists. Skip texture reading.\n");
				}
			}

			newMesh->pMeshConstantsGPU = MeshConstants.pGPU;
			newMesh->pMaterialConstantsGPU = MaterialConstants.pGPU;

			pMeshes.push_back(newMesh);
		}

		// Bounding box 초기화.
		{
			BoundingBox = GetBoundingBox(MESHES[0].Vertices);
			for (size_t i = 1, size = MESHES.size(); i < size; ++i)
			{
				DirectX::BoundingBox bb = GetBoundingBox(MESHES[0].Vertices);
				ExtendBoundingBox(bb, &BoundingBox);
			}
			struct MeshData meshData = INIT_MESH_DATA;
			Geometry::MakeWireBox(&meshData, BoundingBox.Center, Vector3(BoundingBox.Extents) + Vector3(1e-3f));
			m_pBoundingBoxMesh = (struct Mesh*)Malloc(sizeof(struct Mesh));
			*m_pBoundingBoxMesh = INIT_MESH;

			hr = Graphics::CreateVertexBuffer(pDevice, meshData.Vertices, &(m_pBoundingBoxMesh->pVertexBuffer));
			BREAK_IF_FAILED(hr);
			SET_DEBUG_INFO_TO_OBJECT(m_pBoundingBoxMesh->pVertexBuffer, "Model::m_pBoundingBoxMesh->pVertexBuffer");
			m_pBoundingBoxMesh->IndexCount = (UINT)(meshData.Indices.size());
			m_pBoundingBoxMesh->VertexCount = (UINT)(meshData.Vertices.size());
			m_pBoundingBoxMesh->Stride = sizeof(struct Vertex);

			hr = Graphics::CreateIndexBuffer(pDevice, meshData.Indices, &(m_pBoundingBoxMesh->pIndexBuffer));
			BREAK_IF_FAILED(hr);
			SET_DEBUG_INFO_TO_OBJECT(m_pBoundingBoxMesh->pIndexBuffer, "Model::m_pBoundingBoxMesh->pIndexBuffer");
			m_pBoundingBoxMesh->pMeshConstantsGPU = MeshConstants.pGPU;
			m_pBoundingBoxMesh->pMaterialConstantsGPU = MaterialConstants.pGPU;
		}

		// Bounding sphere 초기화.
		{
			float maxRadius = 0.0f;
			for (size_t i = 0, size = MESHES.size(); i < size; ++i)
			{
				const struct MeshData& curMesh = MESHES[i];
				for (size_t j = 0, vertSize = curMesh.Vertices.size(); j < vertSize; ++j)
				{
					const struct Vertex& v = curMesh.Vertices[j];
					maxRadius = std::max((Vector3(BoundingBox.Center) - v.Position).Length(), maxRadius);
				}
			}

			maxRadius += 1e-2f; // 살짝 크게 설정.
			BoundingSphere = DirectX::BoundingSphere(BoundingBox.Center, maxRadius);

			struct MeshData meshData = INIT_MESH_DATA;
			Geometry::MakeWireSphere(&meshData, BoundingSphere.Center, BoundingSphere.Radius);
			m_pBoundingSphereMesh = (struct Mesh*)Malloc(sizeof(struct Mesh));
			*m_pBoundingSphereMesh = INIT_MESH;

			hr = Graphics::CreateVertexBuffer(pDevice, meshData.Vertices, &(m_pBoundingSphereMesh->pVertexBuffer));
			BREAK_IF_FAILED(hr);
			SET_DEBUG_INFO_TO_OBJECT(m_pBoundingSphereMesh->pVertexBuffer, "Model::m_pBoundingSphereMesh->pVertexBuffer");
			m_pBoundingSphereMesh->IndexCount = (UINT)(meshData.Indices.size());
			m_pBoundingSphereMesh->VertexCount = (UINT)(meshData.Vertices.size());
			m_pBoundingSphereMesh->Stride = sizeof(struct Vertex);

			hr = Graphics::CreateIndexBuffer(pDevice, meshData.Indices, &(m_pBoundingSphereMesh->pIndexBuffer));
			BREAK_IF_FAILED(hr);
			SET_DEBUG_INFO_TO_OBJECT(m_pBoundingSphereMesh->pIndexBuffer, "Model::m_pBoundingSphereMesh->pIndexBuffer");
			m_pBoundingSphereMesh->pMeshConstantsGPU = MeshConstants.pGPU;
			m_pBoundingSphereMesh->pMaterialConstantsGPU = MaterialConstants.pGPU;
		}
	}

	void Model::UpdateConstantBuffers(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	{
		if (bIsVisible)
		{
			MeshConstants.Upload(pContext);
			MaterialConstants.Upload(pContext);
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

		MeshConstants.CPU.World = WORLD.Transpose();
		MeshConstants.CPU.WorldInverseTranspose = WorldInverseTranspose.Transpose();
		MeshConstants.CPU.WorldInverse = MeshConstants.CPU.World.Invert();
	}

	void Model::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	{
		OutputDebugStringA("Model::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext) was not implemented.");
		__debugbreak();
	}

	void Model::InitMeshBuffers(ID3D11Device* pDevice,
								const struct MeshData& MESH_DATA,
								struct Mesh* pNewMesh)
	{
		HRESULT hr = S_OK;
		hr = Graphics::CreateVertexBuffer(pDevice, MESH_DATA.Vertices, &(pNewMesh->pVertexBuffer));
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(pNewMesh->pVertexBuffer, "Model::pNewMesh->pVertexBuffer");

		pNewMesh->IndexCount = (UINT)(MESH_DATA.Indices.size());
		pNewMesh->VertexCount = (UINT)(MESH_DATA.Vertices.size());
		pNewMesh->Stride = sizeof(struct Vertex);

		hr = Graphics::CreateIndexBuffer(pDevice, MESH_DATA.Indices, &(pNewMesh->pIndexBuffer));
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(pNewMesh->pIndexBuffer, "Model::pNewMesh->pIndexBuffer");
	}

	Graphics::GraphicsPSO& Model::GetPSO(const bool bWIRED)
	{
		return (bWIRED ? Graphics::g_DefaultWirePSO : Graphics::g_DefaultSolidPSO);
	}

	Graphics::GraphicsPSO& Model::GetDepthOnlyPSO() { return Graphics::g_DepthOnlyPSO; }

	Graphics::GraphicsPSO& Model::GetReflectPSO(const bool bWIRED)
	{
		return (bWIRED ? Graphics::g_ReflectWirePSO : Graphics::g_ReflectSolidPSO);
	}

	void Model::Render(ID3D11DeviceContext* pContext)
	{
		if (bIsVisible)
		{
			for (size_t i = 0, size = pMeshes.size(); i < size; ++i)
			{
				struct Mesh* pMESH = pMeshes[i];

				ID3D11Buffer* ppConstantBuffers[] =
				{
					pMESH->pMeshConstantsGPU,
					pMESH->pMaterialConstantsGPU
				};
				UINT numConstantBuffers = _countof(ppConstantBuffers);

				pContext->VSSetConstantBuffers(1, numConstantBuffers, ppConstantBuffers);
				pContext->VSSetShaderResources(0, 1, &(pMESH->pHeightSRV));

				// 물체 렌더링할 때 여러가지 텍스춰 사용. (t0 부터시작)
				ID3D11ShaderResourceView* ppSRVs[] =
				{
					pMESH->pAlbedoSRV, pMESH->pEmissiveSRV, pMESH->pNormalSRV, 
					pMESH->pAOSRV, pMESH->pMetallicSRV, pMESH->pRoughnessSRV, 
				};
				UINT numSRVs = _countof(ppSRVs);
				pContext->PSSetShaderResources(0, numSRVs, ppSRVs);
				pContext->PSSetConstantBuffers(1, numConstantBuffers, ppConstantBuffers);

				// 볼륨 렌더링.
				ID3D11ShaderResourceView* ppDensitySRV = pMESH->DensityTex.GetSRV();
				ID3D11ShaderResourceView* ppLightingSRV = pMESH->LightingTex.GetSRV();
				if (ppDensitySRV)
				{
					pContext->PSSetShaderResources(6, 1, &ppDensitySRV);
				}
				if (ppLightingSRV)
				{
					pContext->PSSetShaderResources(7, 1, &ppLightingSRV);
				}

				pContext->IASetVertexBuffers(0, 1, &(pMESH->pVertexBuffer), &pMESH->Stride, &pMESH->Offset);
				pContext->IASetIndexBuffer(pMESH->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
				pContext->DrawIndexed(pMESH->IndexCount, 0, 0);

				// Release resources.
				ID3D11ShaderResourceView* ppNulls[3] = { nullptr, nullptr, nullptr };
				pContext->PSSetShaderResources(6, 3, ppNulls);
			}
		}
	}

	void Model::UpdateAnimation(ID3D11DeviceContext* pContext, int clipID, int frame)
	{
		// class SkinnedMeshModel에서 override.
		OutputDebugStringA("Model::UpdateAnimation(ID3D11DeviceContext* pContext, int clipID, int frame) was not implemented.");
		__debugbreak();
	}

	void Model::RenderNormals(ID3D11DeviceContext* pContext)
	{
		for (size_t i = 0, size = pMeshes.size(); i < size; ++i)
		{
			struct Mesh* pMesh = pMeshes[i];

			ID3D11Buffer* ppConstantsBuffers[] =
			{
				pMesh->pMeshConstantsGPU,
				pMesh->pMaterialConstantsGPU
			};
			UINT numConstantsBuffers = _countof(ppConstantsBuffers);
			pContext->GSSetConstantBuffers(1, numConstantsBuffers, ppConstantsBuffers);
			pContext->IASetVertexBuffers(0, 1, &(pMesh->pVertexBuffer), &(pMesh->Stride), &(pMesh->Offset));
			pContext->Draw(pMesh->VertexCount, 0);
		}
	}

	void Model::RenderWireBoundingBox(ID3D11DeviceContext* pContext)
	{
		ID3D11Buffer* ppConstantsBuffers[] =
		{
			m_pBoundingBoxMesh->pMeshConstantsGPU,
			m_pBoundingBoxMesh->pMaterialConstantsGPU
		};
		UINT numConstantsBuffers = _countof(ppConstantsBuffers);
		pContext->VSSetConstantBuffers(1, numConstantsBuffers, ppConstantsBuffers);
		pContext->IASetVertexBuffers(0, 1, &(m_pBoundingBoxMesh->pVertexBuffer), &(m_pBoundingBoxMesh->Stride), &(m_pBoundingBoxMesh->Offset));
		pContext->IASetIndexBuffer(m_pBoundingBoxMesh->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		pContext->DrawIndexed(m_pBoundingBoxMesh->IndexCount, 0, 0);
	}

	void Model::RenderWireBoundingSphere(ID3D11DeviceContext* pContext)
	{
		ID3D11Buffer* ppConstantsBuffers[] =
		{
			m_pBoundingBoxMesh->pMeshConstantsGPU,
			m_pBoundingBoxMesh->pMaterialConstantsGPU
		};
		UINT numConstantBuffers = _countof(ppConstantsBuffers);
		pContext->VSSetConstantBuffers(1, numConstantBuffers, ppConstantsBuffers);
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
