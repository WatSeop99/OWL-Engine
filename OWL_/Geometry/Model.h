#pragma once

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Model/Model.h

#include "Animation.h"
#include "../Graphics/ConstantBuffers.h"
#include "../Graphics/GraphicsCommon.h"
#include "Mesh.h"
#include "MeshInfo.h"

using DirectX::SimpleMath::Matrix;

class Model
{
public:
	Model() = default;
	virtual ~Model() { destroy(); }

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, std::wstring& basePath, std::wstring& fileName);
	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const vector<MeshInfo>& MESH_INFOS);
	virtual void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void InitMeshBuffers(ID3D11Device* pDevice, const MeshInfo& MESH_INFO, Mesh* pNewMesh);

	void UpdateConstantBuffers(ID3D11DeviceContext* pContext);
	void UpdateWorld(const Matrix& WORLD);
	virtual void UpdateAnimation(ID3D11DeviceContext* pContext, int clipID, int frame);

	virtual void Render(ID3D11DeviceContext* pContext);
	virtual void RenderNormals(ID3D11DeviceContext* pContext);
	virtual void RenderWireBoundingBox(ID3D11DeviceContext* pContext);
	virtual void RenderWireBoundingSphere(ID3D11DeviceContext* pContext);

	virtual inline GraphicsPSO& GetPSO(const bool bWIRED) { return (bWIRED ? g_DefaultWirePSO : g_DefaultSolidPSO); }
	virtual inline GraphicsPSO& GetGBufferPSO(const bool bWIRED) { return (bWIRED ? g_GBufferSkinnedPSO : g_GBufferPSO); }
	virtual inline GraphicsPSO& GetDepthOnlyPSO() { return g_DepthOnlyPSO; }
	virtual inline GraphicsPSO& GetDepthOnlyCubePSO() { return g_DepthOnlyCubePSO; }
	virtual inline GraphicsPSO& GetDepthOnlyCascadePSO() { return g_DepthOnlyCascadePSO; }
	virtual inline GraphicsPSO& GetReflectPSO(const bool bWIRED) { return (bWIRED ? g_ReflectWirePSO : g_ReflectSolidPSO); }

protected:
	void destroy();

public:
	Matrix World;					// Model(Object) To World 행렬.
	Matrix WorldInverseTranspose;	// InverseTranspose.

	bool bDrawNormals = false;
	bool bIsVisible = true;
	bool bCastShadow = true;
	bool bIsPickable = false; // 마우스로 선택/조작 가능 여부.

	std::vector<Mesh*> pMeshes;

	DirectX::BoundingBox BoundingBox;
	DirectX::BoundingSphere BoundingSphere;

	std::string Name = "NoName";

private:
	Mesh* m_pBoundingBoxMesh = nullptr;
	Mesh* m_pBoundingSphereMesh = nullptr;
};
