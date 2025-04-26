#include "../Common.h"
#include "Mesh.h"

void Mesh::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	_ASSERT(pDevice);
	_ASSERT(pContext);

	pMaterialBuffer = new Material;
	ZeroMemory(pMaterialBuffer, sizeof(Material));

	MeshConstants initMeshConst;
	MaterialConstants initMaterialConst;
	MeshConstant.Initialize(pDevice, pContext, sizeof(MeshConstants), &initMeshConst);
	MaterialConstant.Initialize(pDevice, pContext, sizeof(MaterialConstants), &initMaterialConst);
}

void Mesh::Cleanup()
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
