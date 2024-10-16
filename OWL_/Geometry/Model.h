#pragma once

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Model/Model.h

#include "../Graphics/EnumData.h"
#include "MeshInfo.h"

class BaseRenderer;
class Mesh;

class Model
{
public:
	Model() = default;
	virtual ~Model() { Cleanup(); }

	void Initialize(BaseRenderer* pRenderer, std::wstring& basePath, std::wstring& fileName);
	void Initialize(BaseRenderer* pRenderer, const std::vector<MeshInfo>& MESH_INFOS);
	virtual void InitMeshBuffers(const MeshInfo& MESH_INFO, Mesh* pNewMesh);

	void UpdateConstantBuffers();
	void UpdateWorld(const  DirectX::SimpleMath::Matrix& WORLD);
	virtual void UpdateAnimation(const int CLIP_ID, const int FRAME);

	virtual void Render();
	virtual void RenderNormals();
	virtual void RenderWireBoundingBox();
	virtual void RenderWireBoundingSphere();

	void Cleanup();

	virtual inline eGraphicsPSOType GetPSO(const bool bWIRED) { return (bWIRED ? GraphicsPSOType_DefaultWire : GraphicsPSOType_DefaultSolid); }
	virtual inline eGraphicsPSOType GetGBufferPSO(const bool bWIRED) { return (bWIRED ? GraphicsPSOType_GBufferWire : GraphicsPSOType_GBuffer); }
	virtual inline eGraphicsPSOType GetDepthOnlyPSO() { return GraphicsPSOType_DepthOnly; }
	virtual inline eGraphicsPSOType GetDepthOnlyCubePSO() { return  GraphicsPSOType_DepthOnlyCube; }
	virtual inline eGraphicsPSOType GetDepthOnlyCascadePSO() { return GraphicsPSOType_DepthOnlyCascade; }
	virtual inline eGraphicsPSOType GetReflectPSO(const bool bWIRED) { return (bWIRED ? GraphicsPSOType_ReflectWire : GraphicsPSOType_ReflectSolid); }

public:
	 DirectX::SimpleMath::Matrix World;					// Model(Object) To World 행렬.
	 DirectX::SimpleMath::Matrix WorldInverseTranspose;	// InverseTranspose.

	bool bDrawNormals = false;
	bool bIsVisible = true;
	bool bCastShadow = true;
	bool bIsPickable = false; // 마우스로 선택/조작 가능 여부.

	std::vector<Mesh*> Meshes;

	DirectX::BoundingBox BoundingBox;
	DirectX::BoundingSphere BoundingSphere;

	std::string Name = "NoName";

protected:
	Mesh* m_pBoundingBoxMesh = nullptr;
	Mesh* m_pBoundingSphereMesh = nullptr;

	// DO NOT release directly.
	BaseRenderer* m_pRenderer = nullptr;
};
