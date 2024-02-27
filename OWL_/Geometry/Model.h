#pragma once

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Model/Model.h

#include "Animation.h"
#include "../Core/ConstantBuffers.h"
#include "../Core/GraphicsCommon.h"
#include "Mesh.h"
#include "MeshInfo.h"

namespace Geometry
{
	using DirectX::SimpleMath::Matrix;

	class Model
	{
	public:
		Model() = default;
		Model(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, std::wstring& basePath, std::wstring& fileName);
		Model(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<MeshInfo>& MESH_INFOS);
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

		virtual inline Graphics::GraphicsPSO& GetPSO(const bool bWIRED) { return (bWIRED ? Graphics::g_DefaultWirePSO : Graphics::g_DefaultSolidPSO); }
		virtual inline Graphics::GraphicsPSO& GetDepthOnlyPSO() { return Graphics::g_DepthOnlyPSO; }
		virtual inline Graphics::GraphicsPSO& GetReflectPSO(const bool bWIRED) { return (bWIRED ? Graphics::g_ReflectWirePSO : Graphics::g_ReflectSolidPSO); }

	protected:
		void destroy();

	public:
		Matrix World;   // Model(Object) To World 행렬.
		Matrix WorldInverseTranspose; // InverseTranspose.

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
}
