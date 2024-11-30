#include <time.h>
#include "../Common.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/ConstantBuffer.h"
#include "GeometryGenerator.h"
#include "Mesh.h"
#include "../Renderer/ResourceManager.h"
#include "../Renderer/Texture.h"
#include "../Renderer/StructuredBuffer.h"
#include "Terrain.h"

int Terrain::ms_TerrainCount = 0;

void Terrain::Initialize(Renderer* pRenderer)
{
	_ASSERT(pRenderer);

	Name = "Terrain";
	Name += std::to_string(ms_TerrainCount++);

	MeshInfo meshInfo;
	//MakeSquareGrid(&meshInfo, 256, 256, 128);
	MakeSquareGrid(&meshInfo, 256, 256, 10);
	Model::Initialize(pRenderer, { meshInfo });

	ID3D11Device* pDevice = pRenderer->GetDevice();
	ID3D11DeviceContext* pContext = pRenderer->GetDeviceContext();
	ResourceManager* pResourceManager = pRenderer->GetResourceManager();

	TerrainConstants initConst = { NoiseScale, Octaves, Persistance, Lacunarity, -10.0f, FLT_MAX };
	m_pTerrainConstantBuffer = new ConstantBuffer;
	m_pTerrainConstantBuffer->Initialize(pDevice, pContext, sizeof(TerrainConstants), &initConst);
	m_pTerrainConstantBuffer->Upload();

	MeshConstants* pMeshConstData = (MeshConstants*)Meshes[0]->MeshConstant.pSystemMem;
	pMeshConstData->bUseHeightMap = TRUE;
	pMeshConstData->bUseColorMap = TRUE;
	Meshes[0]->MeshConstant.Upload();

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = m_ChunkSize;
	desc.Height = m_ChunkSize;
	desc.MipLevels = 0;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R16_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	m_pHeightMap = new Texture;
	m_pHeightMap->Initialize(pDevice, pContext, desc, nullptr, true);

	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	m_pColorMap = new Texture;
	m_pColorMap->Initialize(pDevice, pContext, desc, nullptr, true);


	// octaveoffset 생성.
	m_pOctaveOffset = new StructuredBuffer;

	std::vector<DirectX::SimpleMath::Vector2> octaveOffsets(Octaves);
	srand((UINT)time(nullptr));
	for (int i = 0; i < Octaves; ++i)
	{
		float offsetX = rand() % (100000 + 1 + 100000) - 100000 + Offset.x;
		float offsetY = rand() % (100000 + 1 + 100000) - 100000 + Offset.y;
		octaveOffsets[i] = DirectX::SimpleMath::Vector2(offsetX, offsetY);
	}
	m_pOctaveOffset->Initialize(pDevice, pContext, sizeof(DirectX::SimpleMath::Vector2), Octaves, octaveOffsets.data());

	// Generate height map and color map.
	// set generate terrain resource state.
	ID3D11UnorderedAccessView* ppUAVs[2] = { m_pHeightMap->pUAV, m_pColorMap->pUAV };
	pContext->CSSetConstantBuffers(0, 1, &m_pTerrainConstantBuffer->pBuffer);
	pContext->CSSetShaderResources(0, 1, &m_pOctaveOffset->pSRV);
	pContext->CSSetUnorderedAccessViews(0, 2, ppUAVs, nullptr);

	pResourceManager->SetPipelineState(ComputePSOType_NoiseGenerate);
	pContext->Dispatch(m_ChunkSize / 16, m_ChunkSize / 16, 1);

	// Release resources.
	ID3D11UnorderedAccessView* ppNullUAVs[2] = { nullptr, };
	ID3D11Buffer* pNullCBV = nullptr;
	ID3D11ShaderResourceView* pNullSRV = nullptr;
	pContext->CSSetConstantBuffers(0, 1, &pNullCBV);
	pContext->CSSetShaderResources(0, 1, &pNullSRV);
	pContext->CSSetUnorderedAccessViews(0, 2, ppNullUAVs, nullptr);
}

void Terrain::Update(Renderer* pRenderer)
{
	Cleanup();
	Initialize(pRenderer);
}

void Terrain::Render()
{
	_ASSERT(m_pRenderer);

	ID3D11DeviceContext* pContext = m_pRenderer->GetDeviceContext();

	// Render.
	for (UINT64 i = 0, size = Meshes.size(); i < size; ++i)
	{
		Mesh* const pCurMesh = Meshes[i];

		ID3D11Buffer* ppConstantBuffers[2] = { pCurMesh->MeshConstant.pBuffer, pCurMesh->MaterialConstant.pBuffer };
		pContext->VSSetConstantBuffers(2, 2, ppConstantBuffers);
		pContext->PSSetConstantBuffers(2, 2, ppConstantBuffers);

		// 물체 렌더링할 때 여러가지 텍스춰 사용. (t0 부터시작)
		ID3D11ShaderResourceView* ppVSSRVs[] =
		{
			m_pHeightMap->pSRV,
			m_pColorMap->pSRV,
		};
		ID3D11ShaderResourceView* ppPSSRVs[] =
		{
			pCurMesh->pMaterialBuffer->Albedo.pSRV,
			pCurMesh->pMaterialBuffer->Emissive.pSRV,
			pCurMesh->pMaterialBuffer->Normal.pSRV,
			pCurMesh->pMaterialBuffer->AmbientOcclusion.pSRV,
			pCurMesh->pMaterialBuffer->Metallic.pSRV,
			pCurMesh->pMaterialBuffer->Roughness.pSRV,
		};
		pContext->VSSetShaderResources(6, 2, ppVSSRVs);
		pContext->PSSetShaderResources(0, 6, ppPSSRVs);

		pContext->IASetVertexBuffers(0, 1, &pCurMesh->pVertexBuffer, &pCurMesh->Stride, &pCurMesh->Offset);
		pContext->IASetIndexBuffer(pCurMesh->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		pContext->DrawIndexed(pCurMesh->IndexCount, 0, 0);
	}

	// Release resources.
	ID3D11ShaderResourceView* ppNullVSSRVs[2] = { nullptr, };
	ID3D11ShaderResourceView* ppNullPSSRVs[6] = { nullptr, };
	pContext->VSSetShaderResources(6, 2, ppNullVSSRVs);
	pContext->PSSetShaderResources(0, 6, ppNullPSSRVs);
}

void Terrain::Cleanup()
{
	if (m_pTerrainConstantBuffer)
	{
		delete m_pTerrainConstantBuffer;
		m_pTerrainConstantBuffer = nullptr;
	}
	if (m_pHeightMap)
	{
		delete m_pHeightMap;
		m_pHeightMap = nullptr;
	}
	if (m_pColorMap)
	{
		delete m_pColorMap;
		m_pColorMap = nullptr;
	}
	if (m_pOctaveOffset)
	{
		delete m_pOctaveOffset;
		m_pOctaveOffset = nullptr;
	}

	Model::Cleanup();
}
