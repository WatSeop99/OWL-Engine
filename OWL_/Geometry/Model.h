#pragma once

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Model/Model.h

#include "../Graphics/GraphicsCommon.h"
#include "MeshInfo.h"

using DirectX::SimpleMath::Matrix;

class Mesh;
class BaseRenderer;

class Model
{
public:
	Model() = default;
	virtual ~Model() { Cleanup(); }

	void Initialize(BaseRenderer* pRenderer, std::wstring& basePath, std::wstring& fileName);
	void Initialize(BaseRenderer* pRenderer, const std::vector<MeshInfo>& MESH_INFOS);
	virtual void InitMeshBuffers(const MeshInfo& MESH_INFO, Mesh* pNewMesh);

	void UpdateConstantBuffers();
	void UpdateWorld(const Matrix& WORLD);
	virtual void UpdateAnimation(const int CLIP_ID, const int FRAME);

	virtual void Render();
	virtual void RenderNormals();
	virtual void RenderWireBoundingBox();
	virtual void RenderWireBoundingSphere();

	void Cleanup();

	virtual inline GraphicsPSO& GetPSO(const bool bWIRED) { return (bWIRED ? g_DefaultWirePSO : g_DefaultSolidPSO); }
	virtual inline GraphicsPSO& GetGBufferPSO(const bool bWIRED) { return (bWIRED ? g_GBufferSkinnedPSO : g_GBufferPSO); }
	virtual inline GraphicsPSO& GetDepthOnlyPSO() { return g_DepthOnlyPSO; }
	virtual inline GraphicsPSO& GetDepthOnlyCubePSO() { return g_DepthOnlyCubePSO; }
	virtual inline GraphicsPSO& GetDepthOnlyCascadePSO() { return g_DepthOnlyCascadePSO; }
	virtual inline GraphicsPSO& GetReflectPSO(const bool bWIRED) { return (bWIRED ? g_ReflectWirePSO : g_ReflectSolidPSO); }

public:
	Matrix World;					// Model(Object) To World 행렬.
	Matrix WorldInverseTranspose;	// InverseTranspose.

	bool bDrawNormals = false;
	bool bIsVisible = true;
	bool bCastShadow = true;
	bool bIsPickable = false; // 마우스로 선택/조작 가능 여부.

	std::vector<Mesh*> Meshes;

	DirectX::BoundingBox BoundingBox;
	DirectX::BoundingSphere BoundingSphere;

	std::string Name = "NoName";

protected:
	BaseRenderer* m_pRenderer = nullptr;

	Mesh* m_pBoundingBoxMesh = nullptr;
	Mesh* m_pBoundingSphereMesh = nullptr;
};
