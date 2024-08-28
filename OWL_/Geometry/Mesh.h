#pragma once

#include "../Common.h"
#include "../Renderer/Texture2D.h"
#include "../Renderer/Texture3D.h"

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
class Mesh
{
public:
	Mesh() = default;
	~Mesh() { Cleanup(); };

	void Initialize(ID3D11Device* pDevice)
	{
		_ASSERT(pDevice);

		pMaterialBuffer = New Material;
		*pMaterialBuffer = { Texture2D(), Texture2D(), Texture2D(), Texture2D(), Texture2D(), Texture2D(), Texture2D(), Texture3D(), Texture3D() };
		
		MeshConstants.Initialize(pDevice);
		MaterialConstants.Initialize(pDevice);
	}

	void Cleanup()
	{
		VertexCount = 0;
		IndexCount = 0;
		Stride = 0;
		Offset = 0;

		if (pMaterialBuffer)
		{
			delete pMaterialBuffer;
			pMaterialBuffer = nullptr;
		}
		MeshConstants.Destroy();
		MaterialConstants.Destroy();
		SAFE_RELEASE(pVertexBuffer);
		SAFE_RELEASE(pIndexBuffer);
	}

public:
	ID3D11Buffer* pVertexBuffer = nullptr;
	ID3D11Buffer* pIndexBuffer = nullptr;
	Material* pMaterialBuffer = nullptr;

	ConstantsBuffer<MeshConstants> MeshConstants;
	ConstantsBuffer<MaterialConstants> MaterialConstants;

	UINT VertexCount = 0;
	UINT IndexCount = 0;
	UINT Stride = 0;
	UINT Offset = 0;
};
