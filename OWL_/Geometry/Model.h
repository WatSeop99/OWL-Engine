#pragma once

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Model/Model.h

#include "Animation.h"
#include "../Core/ConstantBuffers.h"

namespace Geometry
{
	using std::cout;
	using std::endl;
	using std::string;
	using std::vector;
	using DirectX::SimpleMath::Matrix;

	class Model
	{
	public:
		Model() :
			World(), WorldInverseTranspose(),
			bDrawNormals(false), bIsVisible(true), bCastShadow(true), bIsPickable(false),
			Name("NoName")
		{ }
		Model(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, std::wstring& basePath, std::wstring& fileName);
		Model(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<struct MeshData>& MESHES);
		virtual ~Model() { destroy(); }

		void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, std::wstring& basePath, std::wstring& fileName);
		void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const vector<struct MeshData>& MESHES);

		void UpdateConstantBuffers(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
		void UpdateWorld(const Matrix& WORLD);

		virtual void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
		virtual void InitMeshBuffers(ID3D11Device* pDevice, const struct MeshData& MESH_DATA, struct Mesh* pNewMesh);
		
		virtual Graphics::GraphicsPSO& GetPSO(const bool bWIRED);
		virtual Graphics::GraphicsPSO& GetDepthOnlyPSO();
		virtual Graphics::GraphicsPSO& GetReflectPSO(const bool bWIRED);

		virtual void Render(ID3D11DeviceContext* pContext);
		virtual void UpdateAnimation(ID3D11DeviceContext* pContext, int clipID, int frame);

		virtual void RenderNormals(ID3D11DeviceContext* pContext);
		virtual void RenderWireBoundingBox(ID3D11DeviceContext* pContext);
		virtual void RenderWireBoundingSphere(ID3D11DeviceContext* pContext);

	protected:
		void destroy();

	public:
		Matrix World;   // Model(Object) To World 행렬.
		Matrix WorldInverseTranspose; // InverseTranspose.

		bool bDrawNormals;
		bool bIsVisible;
		bool bCastShadow;
		bool bIsPickable; // 마우스로 선택/조작 가능 여부.

		std::vector<struct Mesh*> pMeshes;

		Core::ConstantsBuffer<Core::MeshConstants> MeshConstants;
		Core::ConstantsBuffer<Core::MaterialConstants> MaterialConstants;

		DirectX::BoundingBox BoundingBox;
		DirectX::BoundingSphere BoundingSphere;

		std::string Name;

	private:
		struct Mesh* m_pBoundingBoxMesh = nullptr;
		struct Mesh* m_pBoundingSphereMesh = nullptr;
	};

} // namespace hlab
