#pragma once

#include "../Common.h"
#include "../Core/Texture2D.h"
#include "../Core/Texture3D.h"

struct Material
{
	// 2D textures.
	Texture2D Albedo;
	Texture2D Emissive;
	Texture2D Normal;
	Texture2D Height;
	Texture2D AmbientOcclusion;
	Texture2D Metallic;
	Texture2D Roughness;

	// 3D textures.
	Texture3D Density;
	Texture3D Lighting;
};

struct Mesh
{
	ID3D11Buffer* pVertexBuffer = nullptr;
	ID3D11Buffer* pIndexBuffer = nullptr;
	Material* pMaterialBuffer = nullptr;

	UINT VertexCount;
	UINT IndexCount;
	UINT Stride;
	UINT Offset;

	ConstantsBuffer<MeshConstants> MeshConstants;
	ConstantsBuffer<MaterialConstants> MaterialConstants;
};

#define INIT_MESH																					   \
	{																								   \
		nullptr, nullptr, nullptr,																	   \
		0, 0, 0, 0,																					   \
		ConstantsBuffer<MeshConstants>(), ConstantsBuffer<MaterialConstants>() \
	}

#define INIT_MATERIAL																																					 \
	{																																									 \
		Texture2D(), Texture2D(), Texture2D(), Texture2D(), Texture2D(), Texture2D(), Texture2D(), \
		Texture3D(), Texture3D()																													 \
	}

static void ReleaseMesh(Mesh** ppMesh)
{
	_ASSERT(*ppMesh);

	if ((*ppMesh)->pMaterialBuffer != nullptr)
	{
		delete (*ppMesh)->pMaterialBuffer;
		(*ppMesh)->pMaterialBuffer = nullptr;
	}
	SAFE_RELEASE((*ppMesh)->pVertexBuffer);
	SAFE_RELEASE((*ppMesh)->pIndexBuffer);

	(*ppMesh)->MeshConstants.Destroy();
	(*ppMesh)->MaterialConstants.Destroy();

	free(*ppMesh);
	*ppMesh = nullptr;
}