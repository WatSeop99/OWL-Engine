#pragma once

#include "../Common.h"
#include "../Core/Texture3D.h"

namespace Geometry
{
	struct Mesh
	{
		ID3D11Buffer* pVertexBuffer = nullptr;
		ID3D11Buffer* pIndexBuffer = nullptr;

		ID3D11Buffer* pMeshConstantsGPU = nullptr;
		ID3D11Buffer* pMaterialConstantsGPU = nullptr;

		ID3D11Texture2D* pAlbedoTexture = nullptr;
		ID3D11Texture2D* pEmissiveTexture = nullptr;
		ID3D11Texture2D* pNormalTexture = nullptr;
		ID3D11Texture2D* pHeightTexture = nullptr;
		ID3D11Texture2D* pAOTexture = nullptr;
		// ID3D11Texture2D* pMetallicRoughnessTexture = nullptr;
		ID3D11Texture2D* pMetallicTexture = nullptr;
		ID3D11Texture2D* pRoughnessTexture = nullptr;

		ID3D11ShaderResourceView* pAlbedoSRV = nullptr;
		ID3D11ShaderResourceView* pEmissiveSRV = nullptr;
		ID3D11ShaderResourceView* pNormalSRV = nullptr;
		ID3D11ShaderResourceView* pHeightSRV = nullptr;
		ID3D11ShaderResourceView* pAOSRV = nullptr;
		// ID3D11ShaderResourceView* pMetallicRoughnessSRV = nullptr;
		ID3D11ShaderResourceView* pMetallicSRV = nullptr;
		ID3D11ShaderResourceView* pRoughnessSRV = nullptr;

		// 3D Textures.
		Core::Texture3D DensityTex;
		Core::Texture3D LightingTex;

		UINT IndexCount;
		UINT VertexCount;
		UINT Stride;
		UINT Offset;
	};
}

#define INIT_MESH													   \
	{																   \
		nullptr, nullptr,											   \
		nullptr, nullptr,											   \
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, \
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, \
		Core::Texture3D(), Core::Texture3D(),						   \
		0, 0, 0, 0													   \
	}

static void ReleaseMesh(Geometry::Mesh** ppMesh)
{
	_ASSERT(*ppMesh);

	SAFE_RELEASE((*ppMesh)->pVertexBuffer);
	SAFE_RELEASE((*ppMesh)->pIndexBuffer);

	// SAFE_RELEASE(meshConstsGPU);
	// SAFE_RELEASE(materialConstsGPU);
	(*ppMesh)->pMeshConstantsGPU = nullptr;
	(*ppMesh)->pMaterialConstantsGPU = nullptr;

	SAFE_RELEASE((*ppMesh)->pAlbedoTexture);
	SAFE_RELEASE((*ppMesh)->pEmissiveTexture);
	SAFE_RELEASE((*ppMesh)->pNormalTexture);
	SAFE_RELEASE((*ppMesh)->pHeightTexture);
	SAFE_RELEASE((*ppMesh)->pAOTexture);
	// SAFE_RELEASE((*ppMesh)->pMetallicRoughnessTexture);
	SAFE_RELEASE((*ppMesh)->pMetallicTexture);
	SAFE_RELEASE((*ppMesh)->pRoughnessTexture);

	SAFE_RELEASE((*ppMesh)->pAlbedoSRV);
	SAFE_RELEASE((*ppMesh)->pEmissiveSRV);
	SAFE_RELEASE((*ppMesh)->pNormalSRV);
	SAFE_RELEASE((*ppMesh)->pHeightSRV);
	SAFE_RELEASE((*ppMesh)->pAOSRV);
	// SAFE_RELEASE((*ppMesh)->pMetallicRoughnessSRV);
	SAFE_RELEASE((*ppMesh)->pMetallicSRV);
	SAFE_RELEASE((*ppMesh)->pRoughnessSRV);

	free(*ppMesh);
	*ppMesh = nullptr;
}
