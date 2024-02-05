#pragma once

#include "Animation.h"

namespace Geometry
{
	using DirectX::SimpleMath::Vector2;
	using std::string;
	using std::wstring;
	using std::tuple;

	HRESULT ReadFromFile(std::vector<struct MeshData>& dst, std::wstring& basePath, std::wstring& fileName, bool bRevertNormals = false);
	HRESULT ReadAnimationFromFile(std::tuple<std::vector<struct MeshData>, AnimationData>& dst, std::wstring& basePath, std::wstring& fileName, bool bRevertNormals = false);

	void Normalize(const Vector3 CENTER, const float LONGEST_LENGTH, std::vector<struct MeshData>& meshes, AnimationData& animData);

	void MakeSquare(struct MeshData* pDst, const float SCALE = 1.0f, const Vector2 TEX_SCALE = Vector2(1.0f));
	void MakeSquareGrid(struct MeshData* pDst, const int NUM_SLICES, const int NUM_STACKS, const float SCALE = 1.0f, const Vector2 TEX_SCALE = Vector2(1.0f));
	void MakeGrass(struct MeshData* pDst);
	void MakeBox(struct MeshData* pDst, const float SCALE = 1.0f);
	void MakeWireBox(struct MeshData* pDst, const Vector3 CENTER, const Vector3 EXTENTS);
	void MakeWireSphere(struct MeshData* pDst, const Vector3 CENTER, const float RADIUS);
	void MakeCylinder(struct MeshData* pDst, const float BOTTOM_RADIUS, const float TOP_RADIUS, float height, int numSlices);
	void MakeSphere(struct MeshData* pDst, const float RADIUS, const int NUM_SLICES, const int NUM_STACKS, const Vector2 TEX_SCALE = Vector2(1.0f));
	void MakeTetrahedron(struct MeshData* pDst);
	void MakeIcosahedron(struct MeshData* pDst);

	void SubdivideToSphere(struct MeshData* pDst, const float RADIUS, struct MeshData& meshData);
}
