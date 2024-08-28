#include "../Common.h"
#include "GeometryGenerator.h"
#include "../Graphics/GraphicsUtils.h"
#include "Model.h"

DirectX::BoundingBox GetBoundingBox(const std::vector<Vertex>& VERTICES)
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
	ReadFromFile(meshInfos, basePath, fileName);
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
					hr = CreateTexture(pDevice, pContext, MESH_DATA.szAlbedoTextureFileName.c_str(), MESH_DATA.szOpacityTextureFileName.c_str(), false,
												 &(newMesh->pMaterialBuffer->Albedo.pTexture), &(newMesh->pMaterialBuffer->Albedo.pSRV));
				}
				else
				{
					hr = CreateTexture(pDevice, pContext, MESH_DATA.szAlbedoTextureFileName.c_str(), true,
												 &(newMesh->pMaterialBuffer->Albedo.pTexture), &(newMesh->pMaterialBuffer->Albedo.pSRV));
				}
				BREAK_IF_FAILED(hr);
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
				hr = CreateTexture(pDevice, pContext, MESH_DATA.szEmissiveTextureFileName.c_str(), true,
											 &(newMesh->pMaterialBuffer->Emissive.pTexture), &(newMesh->pMaterialBuffer->Emissive.pSRV));
				BREAK_IF_FAILED(hr);
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
				hr = CreateTexture(pDevice, pContext, MESH_DATA.szNormalTextureFileName.c_str(), false,
											 &(newMesh->pMaterialBuffer->Normal.pTexture), &(newMesh->pMaterialBuffer->Normal.pSRV));
				BREAK_IF_FAILED(hr);
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
				hr = CreateTexture(pDevice, pContext, MESH_DATA.szHeightTextureFileName.c_str(), false,
											 &(newMesh->pMaterialBuffer->Height.pTexture), &(newMesh->pMaterialBuffer->Height.pSRV));
				BREAK_IF_FAILED(hr);
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
				hr = CreateTexture(pDevice, pContext, MESH_DATA.szAOTextureFileName.c_str(), false,
											 &(newMesh->pMaterialBuffer->AmbientOcclusion.pTexture), &(newMesh->pMaterialBuffer->AmbientOcclusion.pSRV));
				BREAK_IF_FAILED(hr);
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
				hr = CreateTexture(pDevice, pContext, MESH_DATA.szMetallicTextureFileName.c_str(), false,
											 &(newMesh->pMaterialBuffer->Metallic.pTexture), &(newMesh->pMaterialBuffer->Metallic.pSRV));
				BREAK_IF_FAILED(hr);
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
				hr = CreateTexture(pDevice, pContext, MESH_DATA.szRoughnessTextureFileName.c_str(), false,
											 &(newMesh->pMaterialBuffer->Roughness.pTexture), &(newMesh->pMaterialBuffer->Roughness.pSRV));
				BREAK_IF_FAILED(hr);
				newMesh->MaterialConstants.CPU.bUseRoughnessMap = TRUE;
			}
			else
			{
				OutputDebugStringW(MESH_DATA.szRoughnessTextureFileName.c_str());
				OutputDebugStringA(" does not exists. Skip texture reading.\n");
			}
		}

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
		MakeWireBox(&meshData, BoundingBox.Center, Vector3(BoundingBox.Extents) + Vector3(1e-3f));
		m_pBoundingBoxMesh = (Mesh*)Malloc(sizeof(Mesh));
		*m_pBoundingBoxMesh = INIT_MESH;

		m_pBoundingBoxMesh->pMaterialBuffer = (Material*)Malloc(sizeof(Material));
		*(m_pBoundingBoxMesh->pMaterialBuffer) = INIT_MATERIAL;

		hr = CreateVertexBuffer(pDevice, meshData.Vertices, &(m_pBoundingBoxMesh->pVertexBuffer));
		BREAK_IF_FAILED(hr);

		hr = CreateIndexBuffer(pDevice, meshData.Indices, &(m_pBoundingBoxMesh->pIndexBuffer));
		BREAK_IF_FAILED(hr);

		m_pBoundingBoxMesh->IndexCount = (UINT)(meshData.Indices.size());
		m_pBoundingBoxMesh->VertexCount = (UINT)(meshData.Vertices.size());
		m_pBoundingBoxMesh->Stride = sizeof(Vertex);

		m_pBoundingBoxMesh->MeshConstants.CPU.World = Matrix();
		m_pBoundingBoxMesh->MeshConstants.Initialize(pDevice);
		m_pBoundingBoxMesh->MaterialConstants.Initialize(pDevice);
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
		MakeWireSphere(&meshData, BoundingSphere.Center, BoundingSphere.Radius);
		m_pBoundingSphereMesh = (Mesh*)Malloc(sizeof(Mesh));
		*m_pBoundingSphereMesh = INIT_MESH;

		m_pBoundingSphereMesh->pMaterialBuffer = (Material*)Malloc(sizeof(Material));
		*(m_pBoundingSphereMesh->pMaterialBuffer) = INIT_MATERIAL;

		hr = CreateVertexBuffer(pDevice, meshData.Vertices, &(m_pBoundingSphereMesh->pVertexBuffer));
		BREAK_IF_FAILED(hr);

		hr = CreateIndexBuffer(pDevice, meshData.Indices, &(m_pBoundingSphereMesh->pIndexBuffer));
		BREAK_IF_FAILED(hr);

		m_pBoundingSphereMesh->IndexCount = (UINT)(meshData.Indices.size());
		m_pBoundingSphereMesh->VertexCount = (UINT)(meshData.Vertices.size());
		m_pBoundingSphereMesh->Stride = sizeof(Vertex);

		m_pBoundingSphereMesh->MeshConstants.CPU.World = Matrix();
		m_pBoundingSphereMesh->MeshConstants.Initialize(pDevice);
		m_pBoundingSphereMesh->MaterialConstants.Initialize(pDevice);
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
	hr = CreateVertexBuffer(pDevice, MESH_INFO.Vertices, &(pNewMesh->pVertexBuffer));
	BREAK_IF_FAILED(hr);

	hr = CreateIndexBuffer(pDevice, MESH_INFO.Indices, &(pNewMesh->pIndexBuffer));
	BREAK_IF_FAILED(hr);

	pNewMesh->VertexCount = (UINT)(MESH_INFO.Vertices.size());
	pNewMesh->IndexCount = (UINT)(MESH_INFO.Indices.size());
	pNewMesh->Stride = sizeof(Vertex);
}

void Model::UpdateConstantBuffers(ID3D11DeviceContext* pContext)
{
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

	m_pBoundingBoxMesh->MeshConstants.Upload(pContext);
	m_pBoundingSphereMesh->MeshConstants.Upload(pContext);
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
	BoundingBox.Center = BoundingSphere.Center;

	m_pBoundingBoxMesh->MeshConstants.CPU.World = World.Transpose();
	m_pBoundingBoxMesh->MeshConstants.CPU.WorldInverseTranspose = WorldInverseTranspose.Transpose();
	m_pBoundingBoxMesh->MeshConstants.CPU.WorldInverse = WorldInverseTranspose;
	m_pBoundingSphereMesh->MeshConstants.CPU.World = m_pBoundingBoxMesh->MeshConstants.CPU.World;
	m_pBoundingSphereMesh->MeshConstants.CPU.WorldInverseTranspose = m_pBoundingBoxMesh->MeshConstants.CPU.WorldInverseTranspose;
	m_pBoundingSphereMesh->MeshConstants.CPU.WorldInverse = m_pBoundingBoxMesh->MeshConstants.CPU.WorldInverse;

	for (size_t i = 0, size = pMeshes.size(); i < size; ++i)
	{
		Mesh* pCurMesh = pMeshes[i];
		pCurMesh->MeshConstants.CPU.World = WORLD.Transpose();
		pCurMesh->MeshConstants.CPU.WorldInverseTranspose = WorldInverseTranspose.Transpose();
		pCurMesh->MeshConstants.CPU.WorldInverse = WorldInverseTranspose.Transpose();
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
		pContext->VSSetShaderResources(6, 1, &(pCurMesh->pMaterialBuffer->Height.pSRV));

		// 물체 렌더링할 때 여러가지 텍스춰 사용. (t0 부터시작)
		ID3D11ShaderResourceView* ppSRVs[] =
		{
			pCurMesh->pMaterialBuffer->Albedo.pSRV,
			pCurMesh->pMaterialBuffer->Emissive.pSRV,
			pCurMesh->pMaterialBuffer->Normal.pSRV,
			pCurMesh->pMaterialBuffer->AmbientOcclusion.pSRV,
			pCurMesh->pMaterialBuffer->Metallic.pSRV,
			pCurMesh->pMaterialBuffer->Roughness.pSRV,
			pCurMesh->pMaterialBuffer->Height.pSRV
		};
		UINT numSRVs = _countof(ppSRVs);
		pContext->PSSetShaderResources(0, numSRVs, ppSRVs);
		pContext->PSSetConstantBuffers(2, numConstantBuffers, ppConstantBuffers);

		// 볼륨 렌더링.
		if (pCurMesh->pMaterialBuffer->Density.pSRV != nullptr)
		{
			pContext->PSSetShaderResources(7, 1, &(pCurMesh->pMaterialBuffer->Density.pSRV));
		}
		if (pCurMesh->pMaterialBuffer->Lighting.pSRV != nullptr)
		{
			pContext->PSSetShaderResources(8, 1, &(pCurMesh->pMaterialBuffer->Lighting.pSRV));
		}

		pContext->IASetVertexBuffers(0, 1, &(pCurMesh->pVertexBuffer), &(pCurMesh->Stride), &(pCurMesh->Offset));
		pContext->IASetIndexBuffer(pCurMesh->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		pContext->DrawIndexed(pCurMesh->IndexCount, 0, 0);

		// Release resources.
		ID3D11ShaderResourceView* ppNulls[3] = { nullptr, nullptr, nullptr };
		pContext->PSSetShaderResources(7, 3, ppNulls);
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
