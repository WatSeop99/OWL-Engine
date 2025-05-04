#pragma once

struct MeshInfo;
class AnimationData;

//HRESULT ReadFromFile(std::vector<MeshInfo>& dst, std::wstring& basePath, std::wstring& fileName, bool bRevertNormals = false);
//HRESULT ReadAnimationFromFile(std::tuple<std::vector<MeshInfo>, AnimationData>& dst, std::wstring& basePath, std::wstring& fileName, bool bRevertNormals = false);
HRESULT ReadFromFile(std::vector<MeshInfo>& dst, AnimationData* pAnimData, std::wstring& basePath, std::wstring& fileName, bool bRevertNormals = false);
HRESULT ReadAnimationFromFile(AnimationData* pAnimData, std::wstring& basePath, std::wstring& fileName, bool bRevertNormals = false);


void Normalize(const Vector3& center, float longestLength, std::vector<MeshInfo>& meshes, AnimationData& animData);

void MakeSquare(MeshInfo* pOutDst, float scale = 1.0f, Vector2 texScale = Vector2::One);
void MakeSquareGrid(MeshInfo* pOutDst, int numSlices, int numStacks, float scale = 1.0f, Vector2 texScale = Vector2::One);
void MakeGrass(MeshInfo* pOutDst);
void MakeBox(MeshInfo* pOutDst, float scale = 1.0f);
void MakeWireBox(MeshInfo* pOutDst, Vector3& center, Vector3& extents);
void MakeWireSphere(MeshInfo* pOutDst, Vector3& center, float radius);
void MakeCylinder(MeshInfo* pOutDst, float bottomRadius, float topRadius, float height, int numSlices);
void MakeSphere(MeshInfo* pOutDst, float radius, int numSlices, int numStacks, Vector2 texScale = Vector2::One);
void MakeTetrahedron(MeshInfo* pOutDst);
void MakeIcosahedron(MeshInfo* pOutDst);

void SubdivideToSphere(MeshInfo* pOutDst, float radius, MeshInfo& meshData);
