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

	for (UINT64 i = 1, size = VERTICES.size(); i < size; ++i)
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
	_ASSERT(pDestBox);

	using DirectX::SimpleMath::Vector3;

	Vector3 minCorner = Vector3(SRC_BOX.Center) - Vector3(SRC_BOX.Extents);
	Vector3 maxCorner = Vector3(SRC_BOX.Center) - Vector3(SRC_BOX.Extents);

	minCorner = Min(minCorner, Vector3(pDestBox->Center) - Vector3(pDestBox->Extents));
	maxCorner = Max(maxCorner, Vector3(pDestBox->Center) + Vector3(pDestBox->Extents));

	pDestBox->Center = (minCorner + maxCorner) * 0.5f;
	pDestBox->Extents = maxCorner - pDestBox->Center;
}

void Model::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, std::wstring& basePath, std::wstring& fileName)
{
	std::vector<MeshInfo> meshInfos;
	ReadFromFile(meshInfos, basePath, fileName);
	Initialize(pDevice, pContext, meshInfos);
}

void Model::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const vector<MeshInfo>& MESH_INFOS)
{
	_ASSERT(pDevice);
	_ASSERT(pContext);

	HRESULT hr = S_OK;
	struct _stat64 sourceFileStat;

	Meshes.reserve(MESH_INFOS.size());
	for (UINT64 i = 0, meshSize = MESH_INFOS.size(); i < meshSize; ++i)
	{
		const MeshInfo& MESH_DATA = MESH_INFOS[i];

		Mesh* pNewMesh = new Mesh;
		pNewMesh->Initialize(pDevice);
		InitMeshBuffers(pDevice, MESH_DATA, pNewMesh);
		
		pNewMesh->MeshConstant.CPU.World = Matrix();

		if (!MESH_DATA.szAlbedoTextureFileName.empty())
		{
			std::string albedoTextureA(MESH_DATA.szAlbedoTextureFileName.begin(), MESH_DATA.szAlbedoTextureFileName.end());

			if (_stat64(albedoTextureA.c_str(), &sourceFileStat) != -1)
			{
				if (!MESH_DATA.szOpacityTextureFileName.empty())
				{
					hr = CreateTexture(pDevice, pContext, MESH_DATA.szAlbedoTextureFileName.c_str(), MESH_DATA.szOpacityTextureFileName.c_str(), false,
									   &pNewMesh->pMaterialBuffer->Albedo.pTexture, &pNewMesh->pMaterialBuffer->Albedo.pSRV);
				}
				else
				{
					hr = CreateTexture(pDevice, pContext, MESH_DATA.szAlbedoTextureFileName.c_str(), true,
									   &pNewMesh->pMaterialBuffer->Albedo.pTexture, &pNewMesh->pMaterialBuffer->Albedo.pSRV);
				}
				BREAK_IF_FAILED(hr);
				pNewMesh->MaterialConstant.CPU.bUseAlbedoMap = TRUE;
			}
			else
			{
				WCHAR szDebugString[256];
				swprintf_s(szDebugString, 256, L"%s does not exists. Skip texture reading.\n", MESH_DATA.szAlbedoTextureFileName.c_str());
				OutputDebugStringW(szDebugString);
			}
		}
		if (!MESH_DATA.szEmissiveTextureFileName.empty())
		{
			std::string emissiveTextureA(MESH_DATA.szEmissiveTextureFileName.begin(), MESH_DATA.szEmissiveTextureFileName.end());

			if (_stat64(emissiveTextureA.c_str(), &sourceFileStat) != -1)
			{
				hr = CreateTexture(pDevice, pContext, MESH_DATA.szEmissiveTextureFileName.c_str(), true,
								   &pNewMesh->pMaterialBuffer->Emissive.pTexture, &pNewMesh->pMaterialBuffer->Emissive.pSRV);
				BREAK_IF_FAILED(hr);
				pNewMesh->MaterialConstant.CPU.bUseEmissiveMap = TRUE;
			}
			else
			{
				WCHAR szDebugString[256];
				swprintf_s(szDebugString, 256, L"%s does not exists. Skip texture reading.\n", MESH_DATA.szEmissiveTextureFileName.c_str());
				OutputDebugStringW(szDebugString);
			}
		}
		if (!MESH_DATA.szNormalTextureFileName.empty())
		{
			std::string normalTextureA(MESH_DATA.szNormalTextureFileName.begin(), MESH_DATA.szNormalTextureFileName.end());

			if (_stat64(normalTextureA.c_str(), &sourceFileStat) != -1)
			{
				hr = CreateTexture(pDevice, pContext, MESH_DATA.szNormalTextureFileName.c_str(), false,
								   &pNewMesh->pMaterialBuffer->Normal.pTexture, &pNewMesh->pMaterialBuffer->Normal.pSRV);
				BREAK_IF_FAILED(hr);
				pNewMesh->MaterialConstant.CPU.bUseNormalMap = TRUE;
			}
			else
			{
				WCHAR szDebugString[256];
				swprintf_s(szDebugString, 256, L"%s does not exists. Skip texture reading.\n", MESH_DATA.szNormalTextureFileName.c_str());
				OutputDebugStringW(szDebugString);
			}
		}
		if (!MESH_DATA.szHeightTextureFileName.empty())
		{
			std::string heightTextureA(MESH_DATA.szHeightTextureFileName.begin(), MESH_DATA.szHeightTextureFileName.end());

			if (_stat64(heightTextureA.c_str(), &sourceFileStat) != -1)
			{
				hr = CreateTexture(pDevice, pContext, MESH_DATA.szHeightTextureFileName.c_str(), false,
								   &pNewMesh->pMaterialBuffer->Height.pTexture, &pNewMesh->pMaterialBuffer->Height.pSRV);
				BREAK_IF_FAILED(hr);
				pNewMesh->MeshConstant.CPU.bUseHeightMap = TRUE;
			}
			else
			{
				WCHAR szDebugString[256];
				swprintf_s(szDebugString, 256, L"%s does not exists. Skip texture reading.\n", MESH_DATA.szHeightTextureFileName.c_str());
				OutputDebugStringW(szDebugString);
			}
		}
		if (!MESH_DATA.szAOTextureFileName.empty())
		{
			std::string aoTextureA(MESH_DATA.szAOTextureFileName.begin(), MESH_DATA.szAOTextureFileName.end());

			if (_stat64(aoTextureA.c_str(), &sourceFileStat) != -1)
			{
				hr = CreateTexture(pDevice, pContext, MESH_DATA.szAOTextureFileName.c_str(), false,
								   &pNewMesh->pMaterialBuffer->AmbientOcclusion.pTexture, &pNewMesh->pMaterialBuffer->AmbientOcclusion.pSRV);
				BREAK_IF_FAILED(hr);
				pNewMesh->MaterialConstant.CPU.bUseAOMap = TRUE;
			}
			else
			{
				WCHAR szDebugString[256];
				swprintf_s(szDebugString, 256, L"%s does not exists. Skip texture reading.\n", MESH_DATA.szAOTextureFileName.c_str());
				OutputDebugStringW(szDebugString);
			}
		}
		if (!MESH_DATA.szMetallicTextureFileName.empty())
		{
			std::string metallicTextureA(MESH_DATA.szMetallicTextureFileName.begin(), MESH_DATA.szMetallicTextureFileName.end());

			if (_stat64(metallicTextureA.c_str(), &sourceFileStat) != -1)
			{
				hr = CreateTexture(pDevice, pContext, MESH_DATA.szMetallicTextureFileName.c_str(), false,
								   &pNewMesh->pMaterialBuffer->Metallic.pTexture, &pNewMesh->pMaterialBuffer->Metallic.pSRV);
				BREAK_IF_FAILED(hr);
				pNewMesh->MaterialConstant.CPU.bUseMetallicMap = TRUE;
			}
			else
			{
				WCHAR szDebugString[256];
				swprintf_s(szDebugString, 256, L"%s does not exists. Skip texture reading.\n", MESH_DATA.szMetallicTextureFileName.c_str());
				OutputDebugStringW(szDebugString);
			}
		}
		if (!MESH_DATA.szRoughnessTextureFileName.empty())
		{
			std::string roughnessTextureA(MESH_DATA.szRoughnessTextureFileName.begin(), MESH_DATA.szRoughnessTextureFileName.end());

			if (_stat64(roughnessTextureA.c_str(), &sourceFileStat) != -1)
			{
				hr = CreateTexture(pDevice, pContext, MESH_DATA.szRoughnessTextureFileName.c_str(), false,
								   &pNewMesh->pMaterialBuffer->Roughness.pTexture, &pNewMesh->pMaterialBuffer->Roughness.pSRV);
				BREAK_IF_FAILED(hr);
				pNewMesh->MaterialConstant.CPU.bUseRoughnessMap = TRUE;
			}
			else
			{
				WCHAR szDebugString[256];
				swprintf_s(szDebugString, 256, L"%s does not exists. Skip texture reading.\n", MESH_DATA.szRoughnessTextureFileName.c_str());
				OutputDebugStringW(szDebugString);
			}
		}

		Meshes.push_back(pNewMesh);
	}

	// Bounding box 초기화.
	{
		BoundingBox = GetBoundingBox(MESH_INFOS[0].Vertices);
		for (UINT64 i = 1, size = MESH_INFOS.size(); i < size; ++i)
		{
			DirectX::BoundingBox bb = GetBoundingBox(MESH_INFOS[0].Vertices);
			ExtendBoundingBox(bb, &BoundingBox);
		}

		MeshInfo meshData;
		MakeWireBox(&meshData, BoundingBox.Center, Vector3(BoundingBox.Extents) + Vector3(1e-3f));
		m_pBoundingBoxMesh = New Mesh;
		m_pBoundingBoxMesh->Initialize(pDevice);

		m_pBoundingBoxMesh->MeshConstant.CPU.World = Matrix();

		hr = CreateVertexBuffer(pDevice, meshData.Vertices, &m_pBoundingBoxMesh->pVertexBuffer);
		BREAK_IF_FAILED(hr);

		hr = CreateIndexBuffer(pDevice, meshData.Indices, &m_pBoundingBoxMesh->pIndexBuffer);
		BREAK_IF_FAILED(hr);

		m_pBoundingBoxMesh->IndexCount = (UINT)meshData.Indices.size();
		m_pBoundingBoxMesh->VertexCount = (UINT)meshData.Vertices.size();
		m_pBoundingBoxMesh->Stride = sizeof(Vertex);
	}

	// Bounding sphere 초기화.
	{
		float maxRadius = 0.0f;
		for (UINT64 i = 0, size = MESH_INFOS.size(); i < size; ++i)
		{
			const MeshInfo& curMesh = MESH_INFOS[i];
			for (UINT64 j = 0, vertSize = curMesh.Vertices.size(); j < vertSize; ++j)
			{
				const Vertex& v = curMesh.Vertices[j];
				maxRadius = std::max((Vector3(BoundingBox.Center) - v.Position).Length(), maxRadius);
			}
		}

		maxRadius += 1e-2f; // 살짝 크게 설정.
		BoundingSphere = DirectX::BoundingSphere(BoundingBox.Center, maxRadius);

		MeshInfo meshData;
		MakeWireSphere(&meshData, BoundingSphere.Center, BoundingSphere.Radius);
		m_pBoundingSphereMesh = New Mesh;
		m_pBoundingSphereMesh->Initialize(pDevice);

		m_pBoundingSphereMesh->MeshConstant.CPU.World = Matrix();

		hr = CreateVertexBuffer(pDevice, meshData.Vertices, &(m_pBoundingSphereMesh->pVertexBuffer));
		BREAK_IF_FAILED(hr);

		hr = CreateIndexBuffer(pDevice, meshData.Indices, &(m_pBoundingSphereMesh->pIndexBuffer));
		BREAK_IF_FAILED(hr);

		m_pBoundingSphereMesh->IndexCount = (UINT)(meshData.Indices.size());
		m_pBoundingSphereMesh->VertexCount = (UINT)(meshData.Vertices.size());
		m_pBoundingSphereMesh->Stride = sizeof(Vertex);
	}
}

void Model::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	OutputDebugStringA("Model::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext) was not implemented.");
	__debugbreak();
}

void Model::InitMeshBuffers(ID3D11Device* pDevice, const MeshInfo& MESH_INFO, Mesh* pNewMesh)
{
	_ASSERT(pDevice);
	_ASSERT(pNewMesh);

	HRESULT hr = S_OK;

	hr = CreateVertexBuffer(pDevice, MESH_INFO.Vertices, &pNewMesh->pVertexBuffer);
	BREAK_IF_FAILED(hr);

	hr = CreateIndexBuffer(pDevice, MESH_INFO.Indices, &pNewMesh->pIndexBuffer);
	BREAK_IF_FAILED(hr);

	pNewMesh->VertexCount = (UINT)MESH_INFO.Vertices.size();
	pNewMesh->IndexCount = (UINT)MESH_INFO.Indices.size();
	pNewMesh->Stride = sizeof(Vertex);
}

void Model::UpdateConstantBuffers(ID3D11DeviceContext* pContext)
{
	_ASSERT(pContext);

	if (bIsVisible == false)
	{
		return;
	}

	for (UINT64 i = 0, size = Meshes.size(); i < size; ++i)
	{
		Mesh* pCurMesh = Meshes[i];
		pCurMesh->MeshConstant.Upload(pContext);
		pCurMesh->MaterialConstant.Upload(pContext);
	}

	m_pBoundingBoxMesh->MeshConstant.Upload(pContext);
	m_pBoundingSphereMesh->MeshConstant.Upload(pContext);
}

void Model::UpdateWorld(const Matrix& WORLD)
{
	World = WORLD;
	WorldInverseTranspose = WORLD;
	// WorldInverseTranspose.Translation(Vector3(0.0f));
	WorldInverseTranspose = WorldInverseTranspose.Invert().Transpose();

	// bounding sphere 위치 업데이트.
	// 스케일까지 고려하고 싶다면 x, y, z 스케일 중 가장 큰 값으로 스케일.
	// 구(sphere)라서 회전은 고려할 필요 없음.
	BoundingSphere.Center = World.Translation();
	BoundingBox.Center = BoundingSphere.Center;

	m_pBoundingBoxMesh->MeshConstant.CPU.World = World.Transpose();
	m_pBoundingBoxMesh->MeshConstant.CPU.WorldInverseTranspose = WorldInverseTranspose.Transpose();
	m_pBoundingBoxMesh->MeshConstant.CPU.WorldInverse = WorldInverseTranspose;
	m_pBoundingSphereMesh->MeshConstant.CPU.World = m_pBoundingBoxMesh->MeshConstant.CPU.World;
	m_pBoundingSphereMesh->MeshConstant.CPU.WorldInverseTranspose = m_pBoundingBoxMesh->MeshConstant.CPU.WorldInverseTranspose;
	m_pBoundingSphereMesh->MeshConstant.CPU.WorldInverse = m_pBoundingBoxMesh->MeshConstant.CPU.WorldInverse;

	for (UINT64 i = 0, size = Meshes.size(); i < size; ++i)
	{
		Mesh* pCurMesh = Meshes[i];
		pCurMesh->MeshConstant.CPU.World = WORLD.Transpose();
		pCurMesh->MeshConstant.CPU.WorldInverseTranspose = WorldInverseTranspose.Transpose();
		pCurMesh->MeshConstant.CPU.WorldInverse = WorldInverseTranspose;
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
	_ASSERT(pContext);

	if (bIsVisible == false)
	{
		return;
	}

	for (UINT64 i = 0, size = Meshes.size(); i < size; ++i)
	{
		Mesh* const pCurMesh = Meshes[i];

		ID3D11Buffer* ppConstantBuffers[] = { pCurMesh->MeshConstant.pGPU, pCurMesh->MaterialConstant.pGPU };
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
		if (pCurMesh->pMaterialBuffer->Density.pSRV)
		{
			pContext->PSSetShaderResources(7, 1, &pCurMesh->pMaterialBuffer->Density.pSRV);
		}
		if (pCurMesh->pMaterialBuffer->Lighting.pSRV)
		{
			pContext->PSSetShaderResources(8, 1, &pCurMesh->pMaterialBuffer->Lighting.pSRV);
		}

		pContext->IASetVertexBuffers(0, 1, &pCurMesh->pVertexBuffer, &pCurMesh->Stride, &pCurMesh->Offset);
		pContext->IASetIndexBuffer(pCurMesh->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		pContext->DrawIndexed(pCurMesh->IndexCount, 0, 0);

		// Release resources.
		ID3D11ShaderResourceView* ppNulls[3] = { nullptr, nullptr, nullptr };
		pContext->PSSetShaderResources(7, 3, ppNulls);
	}
}

void Model::RenderNormals(ID3D11DeviceContext* pContext)
{
	_ASSERT(pContext);

	for (UINT64 i = 0, size = Meshes.size(); i < size; ++i)
	{
		Mesh* const pCurMesh = Meshes[i];

		ID3D11Buffer* ppConstantsBuffers[] = { pCurMesh->MeshConstant.pGPU, pCurMesh->MaterialConstant.pGPU };
		UINT numConstantsBuffers = _countof(ppConstantsBuffers);
		pContext->GSSetConstantBuffers(2, numConstantsBuffers, ppConstantsBuffers);
		pContext->IASetVertexBuffers(0, 1, &pCurMesh->pVertexBuffer, &pCurMesh->Stride, &pCurMesh->Offset);
		pContext->Draw(pCurMesh->VertexCount, 0);
	}
}

void Model::RenderWireBoundingBox(ID3D11DeviceContext* pContext)
{
	_ASSERT(pContext);

	ID3D11Buffer* ppConstantsBuffers[] = { m_pBoundingBoxMesh->MeshConstant.pGPU, m_pBoundingBoxMesh->MaterialConstant.pGPU };
	UINT numConstantsBuffers = _countof(ppConstantsBuffers);
	pContext->VSSetConstantBuffers(2, numConstantsBuffers, ppConstantsBuffers);
	pContext->IASetVertexBuffers(0, 1, &m_pBoundingBoxMesh->pVertexBuffer, &m_pBoundingBoxMesh->Stride, &m_pBoundingBoxMesh->Offset);
	pContext->IASetIndexBuffer(m_pBoundingBoxMesh->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	pContext->DrawIndexed(m_pBoundingBoxMesh->IndexCount, 0, 0);
}

void Model::RenderWireBoundingSphere(ID3D11DeviceContext* pContext)
{
	_ASSERT(pContext);

	ID3D11Buffer* ppConstantsBuffers[] = { m_pBoundingSphereMesh->MeshConstant.pGPU, m_pBoundingSphereMesh->MaterialConstant.pGPU };
	UINT numConstantBuffers = _countof(ppConstantsBuffers);
	pContext->VSSetConstantBuffers(2, numConstantBuffers, ppConstantsBuffers);
	pContext->IASetVertexBuffers(0, 1, &m_pBoundingSphereMesh->pVertexBuffer, &m_pBoundingSphereMesh->Stride, &m_pBoundingSphereMesh->Offset);
	pContext->IASetIndexBuffer(m_pBoundingSphereMesh->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	pContext->DrawIndexed(m_pBoundingSphereMesh->IndexCount, 0, 0);
}

void Model::Cleanup()
{
	if (m_pBoundingSphereMesh)
	{
		delete m_pBoundingSphereMesh;
		m_pBoundingSphereMesh = nullptr;
	}
	if (m_pBoundingBoxMesh)
	{
		delete m_pBoundingBoxMesh;
		m_pBoundingBoxMesh = nullptr;
	}

	for (UINT64 i = 0, size = Meshes.size(); i < size; ++i)
	{
		delete Meshes[i];
		Meshes[i] = nullptr;
	}
	Meshes.clear();
}
