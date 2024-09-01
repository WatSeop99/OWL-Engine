#pragma once

#include "../Common.h"
#include "../Graphics/ConstantDataType.h"
#include "../Renderer/ConstantBuffer.h"
#include "../Renderer/Texture.h"
#include "../Renderer/Texture2D.h"
#include "../Renderer/Texture3D.h"

struct Material
{
	// 2D textures.
	/*Texture2D Albedo;
	Texture2D Emissive;
	Texture2D Normal;
	Texture2D Height;
	Texture2D AmbientOcclusion;
	Texture2D Metallic;
	Texture2D Roughness;*/
	Texture Albedo;
	Texture Emissive;
	Texture Normal;
	Texture Height;
	Texture AmbientOcclusion;
	Texture Metallic;
	Texture Roughness;

	// 3D textures.
	/*Texture3D Density;
	Texture3D Lighting;*/
	Texture Density;
	Texture Lighting;
};
class Mesh
{
public:
	Mesh() = default;
	~Mesh() { Cleanup(); };

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	{
		_ASSERT(pDevice);
		_ASSERT(pContext);

		pMaterialBuffer = New Material;
		ZeroMemory(pMaterialBuffer, sizeof(Material));
		//*pMaterialBuffer = { Texture(), Texture(), Texture(), Texture(), Texture(), Texture(), Texture(), Texture(), Texture() };
		//*pMaterialBuffer = { Texture2D(), Texture2D(), Texture2D(), Texture2D(), Texture2D(), Texture2D(), Texture2D(), Texture3D(), Texture3D() };
		
		MeshConstants initMeshConst;
		MaterialConstants initMaterialConst;
		MeshConstant.Initialize(pDevice, pContext, sizeof(MeshConstants), &initMeshConst);
		MaterialConstant.Initialize(pDevice, pContext, sizeof(MaterialConstants), &initMaterialConst);
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
		MeshConstant.Cleanup();
		MaterialConstant.Cleanup();
		SAFE_RELEASE(pVertexBuffer);
		SAFE_RELEASE(pIndexBuffer);
	}

public:
	ID3D11Buffer* pVertexBuffer = nullptr;
	ID3D11Buffer* pIndexBuffer = nullptr;
	Material* pMaterialBuffer = nullptr;

	ConstantBuffer MeshConstant;
	ConstantBuffer MaterialConstant;

	UINT VertexCount = 0;
	UINT IndexCount = 0;
	UINT Stride = 0;
	UINT Offset = 0;
};
