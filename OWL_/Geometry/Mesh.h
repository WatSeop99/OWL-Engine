#pragma once

#include "../Common.h"
#include "../Graphics/ConstantDataType.h"
#include "../Renderer/ConstantBuffer.h"
#include "../Renderer/Texture.h"

struct Material
{
	// 2D textures.
	Texture Albedo;
	Texture Emissive;
	Texture Normal;
	Texture Height;
	Texture AmbientOcclusion;
	Texture Metallic;
	Texture Roughness;

	// 3D textures.
	Texture Density;
	Texture Lighting;
};
class Mesh
{
public:
	Mesh() = default;
	~Mesh() { Cleanup(); };

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

	void Cleanup();

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
