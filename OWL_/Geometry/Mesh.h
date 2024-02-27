#pragma once

#include "../Common.h"
#include "../Core/Texture2D.h"
#include "../Core/Texture3D.h"

namespace Geometry
{
	struct Material
	{
		// 2D textures.
		Graphics::Texture2D Albedo;
		Graphics::Texture2D Emissive;
		Graphics::Texture2D Normal;
		Graphics::Texture2D Height;
		Graphics::Texture2D AmbientOcclusion;
		Graphics::Texture2D Metallic;
		Graphics::Texture2D Roughness;

		// 3D textures.
		Graphics::Texture3D Density;
		Graphics::Texture3D Lighting;
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

		Core::ConstantsBuffer<Core::MeshConstants> MeshConstants;
		Core::ConstantsBuffer<Core::MaterialConstants> MaterialConstants;
	};
}

#define INIT_MESH																					   \
	{																								   \
		nullptr, nullptr, nullptr,																	   \
		0, 0, 0, 0,																					   \
		Core::ConstantsBuffer<Core::MeshConstants>(), Core::ConstantsBuffer<Core::MaterialConstants>() \
	}

#define INIT_MATERIAL																																					 \
	{																																									 \
		Graphics::Texture2D(), Graphics::Texture2D(), Graphics::Texture2D(), Graphics::Texture2D(), Graphics::Texture2D(), Graphics::Texture2D(), Graphics::Texture2D(), \
		Graphics::Texture3D(), Graphics::Texture3D()																													 \
	}

static void ReleaseMesh(Geometry::Mesh** ppMesh)
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
