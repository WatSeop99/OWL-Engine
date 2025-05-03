#include "../Common.h"
#include "Animation.h"
#include "MeshInfo.h"
#include "ModelLoader.h"
#include "GeometryGenerator.h"

HRESULT ReadFromFile(std::vector<MeshInfo>& dst, std::wstring& basePath, std::wstring& fileName, bool bRevertNormals)
{
	HRESULT hr = S_OK;

	ModelLoader modelLoader;
	hr = modelLoader.Load(basePath, fileName, bRevertNormals);
	if (FAILED(hr))
	{
		goto LB_RET;
	}

	Normalize(Vector3::Zero, 1.0f, modelLoader.MeshInfos, modelLoader.AnimData);
	dst = modelLoader.MeshInfos;

LB_RET:
	return hr;
}

HRESULT ReadAnimationFromFile(std::tuple<std::vector<MeshInfo>, AnimationData>& dst, std::wstring& basePath, std::wstring& fileName, bool bRevertNormals)
{
	HRESULT hr = S_OK;

	ModelLoader modelLoader;
	hr = modelLoader.Load(basePath, fileName, bRevertNormals);
	if (FAILED(hr))
	{
		goto LB_RET;
	}

	Normalize(Vector3::Zero, 1.0f, modelLoader.MeshInfos, modelLoader.AnimData);
	dst = { modelLoader.MeshInfos, modelLoader.AnimData };

LB_RET:
	return hr;
}

HRESULT ReadFromFile(std::vector<MeshInfo>& dst, AnimationData* pAnimData, std::wstring& basePath, std::wstring& fileName, bool bRevertNormals)
{
	HRESULT hr = S_OK;

	ModelLoader modelLoader;
	hr = modelLoader.Load(basePath, fileName, bRevertNormals);
	if (FAILED(hr))
	{
		hr = E_FAIL;
		goto LB_RET;
	}

	Normalize(Vector3::Zero, 1.0f, modelLoader.MeshInfos, modelLoader.AnimData);
	dst = modelLoader.MeshInfos;
	if (pAnimData)
	{
		*pAnimData = modelLoader.AnimData;
	}

LB_RET:
	return hr;
}

HRESULT ReadAnimationFromFile(AnimationData* pAnimData, std::wstring& basePath, std::wstring& fileName, bool bRevertNormals)
{
	HRESULT hr = S_OK;

	ModelLoader modelLoader;
	hr = modelLoader.LoadAnimation(basePath, fileName);
	if (FAILED(hr))
	{
		hr = E_FAIL;
		goto LB_RET;
	}

	if (pAnimData)
	{
		*pAnimData = modelLoader.AnimData;
	}

LB_RET:
	return hr;
}

void Normalize(const Vector3& center, float longestLength, std::vector<MeshInfo>& meshes, AnimationData& animData)
{
	// 모델의 중심을 원점으로 옮기고 크기를 [-1,1]^3으로 스케일 -> 박스 형태로.

	// Normalize vertices
	Vector3 minVector(1000.0f);
	Vector3 maxVector(-1000.0f);
	for (SIZE_T i = 0, totalMesh = meshes.size(); i < totalMesh; ++i)
	{
		MeshInfo& curMesh = meshes[i];
		for (SIZE_T j = 0, vertSize = curMesh.Vertices.size(); j < vertSize; ++j)
		{
			Vertex& v = curMesh.Vertices[j];
			minVector = Min(minVector, v.Position);
			maxVector = Max(maxVector, v.Position);
		}
	}

	Vector3 delta = maxVector - minVector;
	float scale = longestLength / DirectX::XMMax(DirectX::XMMax(delta.x, delta.y), delta.z);
	Vector3 translation = -(minVector + maxVector) * 0.5f + center;

	for (SIZE_T i = 0, totalMesh = meshes.size(); i < totalMesh; ++i)
	{
		MeshInfo& curMesh = meshes[i];
		for (SIZE_T j = 0, vertSize = curMesh.Vertices.size(); j < vertSize; ++j)
		{
			Vertex& v = curMesh.Vertices[j];
			v.Position = (v.Position + translation) * scale;
		}
		for (SIZE_T j = 0, skinnedVertSize = curMesh.SkinnedVertices.size(); j < skinnedVertSize; ++j)
		{
			SkinnedVertex& v = curMesh.SkinnedVertices[j];
			v.Position = (v.Position + translation) * scale;
		}
	}

	// 애니메이션 데이터 보정에 사용.
	animData.DefaultTransform = Matrix::CreateTranslation(translation) * Matrix::CreateScale(scale);
	animData.InverseDefaultTransform = animData.DefaultTransform.Invert();
}

void MakeSquare(MeshInfo* pOutDst, float scale, Vector2 texScale)
{
	// Texture Coordinates (Direct3D 9)
	// https://learn.microsoft.com/en-us/windows/win32/direct3d9/texture-coordinates

	_ASSERT(pOutDst);

	pOutDst->Vertices.resize(4);

	Vertex& v0 = pOutDst->Vertices[0];
	Vertex& v1 = pOutDst->Vertices[1];
	Vertex& v2 = pOutDst->Vertices[2];
	Vertex& v3 = pOutDst->Vertices[3];

	v0.Position = Vector3(-1.0f, 1.0f, 0.0f) * scale;
	v0.Normal = -Vector3::UnitZ;
	v0.Texcoord = Vector2::Zero * texScale;
	v0.Tangent = Vector3::UnitX;

	v1.Position = Vector3(1.0f, 1.0f, 0.0f) * scale;
	v1.Normal = -Vector3::UnitZ;
	v1.Texcoord = Vector2::UnitX * texScale;
	v1.Tangent = Vector3::UnitX;

	v2.Position = Vector3(1.0f, -1.0f, 0.0f) * scale;
	v2.Normal = -Vector3::UnitZ;
	v2.Texcoord = Vector2::One * texScale;
	v2.Tangent = Vector3::UnitX;

	v3.Position = Vector3(-1.0f, -1.0f, 0.0f) * scale;
	v3.Normal = -Vector3::UnitZ;
	v3.Texcoord = Vector2::UnitY * texScale;
	v3.Tangent = Vector3::UnitX;

	pOutDst->Indices = { 0, 1, 2, 0, 2, 3, };
}

void MakeSquareGrid(MeshInfo* pOutDst, int numSlices, int numStacks, float scale, Vector2 texScale)
{
	_ASSERT(pOutDst);

	pOutDst->Vertices.resize((numStacks + 1) * (numSlices + 1));
	pOutDst->Indices.reserve(numStacks * numSlices * 6);

	float dx = 2.0f / numSlices;
	float dy = 2.0f / numStacks;

	float y = 1.0f;
	for (int j = 0; j < numStacks + 1; ++j)
	{
		float x = -1.0f;
		for (int i = 0; i < numSlices + 1; ++i)
		{
			Vertex& v = pOutDst->Vertices[j * (numSlices + 1) + i];
			//v.Position = Vector3(x, y, 0.0f) * SCALE;
			v.Position = Vector3(x, 0.0f, y) * scale;
			//v.Normal = Vector3(0.0f, 0.0f, -1.0f);
			v.Normal = Vector3::UnitY;
			v.Texcoord = Vector2(x + 1.0f, y + 1.0f) * 0.5f * texScale;
			v.Tangent = Vector3::UnitX;

			x += dx;
		}
		y -= dy;
	}

	for (int j = 0; j < numStacks; ++j)
	{
		for (int i = 0; i < numSlices; ++i)
		{
			pOutDst->Indices.push_back((numSlices + 1) * j + i);
			pOutDst->Indices.push_back((numSlices + 1) * j + i + 1);
			pOutDst->Indices.push_back((numSlices + 1) * (j + 1) + i);

			pOutDst->Indices.push_back((numSlices + 1) * (j + 1) + i);
			pOutDst->Indices.push_back((numSlices + 1) * j + i + 1);
			pOutDst->Indices.push_back((numSlices + 1) * (j + 1) + i + 1);
		}
	}
}

void MakeGrass(MeshInfo* pOutDst)
{
	_ASSERT(pOutDst);

	MakeSquareGrid(pOutDst, 1, 4);

	for (SIZE_T i = 0, size = pOutDst->Vertices.size(); i < size; ++i)
	{
		Vertex& v = pOutDst->Vertices[i];

		// 적당히 가늘게 조절.
		v.Position.x *= 0.02f;

		// Y범위를 0~1로 조절.
		v.Position.y = v.Position.y * 0.5f + 0.5f;
	}

	// 맨 위를 뾰족하게 만들기 위해 삼각형 하나와 정점 하나 삭제.
	pOutDst->Indices.erase(pOutDst->Indices.begin(), pOutDst->Indices.begin() + 3);
	for (SIZE_T i = 0, size = pOutDst->Indices.size(); i < size; ++i)
	{
		pOutDst->Indices[i] -= 1;
	}
	pOutDst->Vertices.erase(pOutDst->Vertices.begin());
	pOutDst->Vertices[0].Position.x = 0.0f;
	pOutDst->Vertices[0].Texcoord.x = 0.5f;
}

void MakeBox(MeshInfo* pOutDst, float scale)
{
	_ASSERT(pOutDst);

	pOutDst->Vertices.resize(24);

	// 윗면
	Vertex& v0 = pOutDst->Vertices[0];
	Vertex& v1 = pOutDst->Vertices[1];
	Vertex& v2 = pOutDst->Vertices[2];
	Vertex& v3 = pOutDst->Vertices[3];

	v0.Position = Vector3(-1.0f, 1.0f, -1.0f) * scale;
	v0.Normal = Vector3::UnitY;
	v0.Texcoord = Vector2::Zero;

	v1.Position = Vector3(-1.0f, 1.0f, 1.0f) * scale;
	v1.Normal = Vector3::UnitY;
	v1.Texcoord = Vector2::UnitX;

	v2.Position = Vector3::One * scale;
	v2.Normal = Vector3::UnitY;
	v2.Texcoord = Vector2::One;

	v3.Position = Vector3(1.0f, 1.0f, -1.0f) * scale;
	v3.Normal = Vector3::UnitY;
	v3.Texcoord = Vector2::UnitY;

	// 아랫면
	Vertex& v4 = pOutDst->Vertices[4];
	Vertex& v5 = pOutDst->Vertices[5];
	Vertex& v6 = pOutDst->Vertices[6];
	Vertex& v7 = pOutDst->Vertices[7];

	v4.Position = (-Vector3::One) * scale;
	v4.Normal = -Vector3::UnitY;
	v4.Texcoord = Vector2::Zero;

	v5.Position = Vector3(1.0f, -1.0f, -1.0f) * scale;
	v5.Normal = -Vector3::UnitY;
	v5.Texcoord = Vector2::UnitX;

	v6.Position = Vector3(1.0f, -1.0f, 1.0f) * scale;
	v6.Normal = -Vector3::UnitY;
	v6.Texcoord = Vector2::One;

	v7.Position = Vector3(-1.0f, -1.0f, 1.0f) * scale;
	v7.Normal = -Vector3::UnitY;
	v7.Texcoord = Vector2::UnitY;

	// 앞면
	Vertex& v8 = pOutDst->Vertices[8];
	Vertex& v9 = pOutDst->Vertices[9];
	Vertex& v10 = pOutDst->Vertices[10];
	Vertex& v11 = pOutDst->Vertices[11];

	v8.Position = (-Vector3::One) * scale;
	v8.Normal = -Vector3::UnitZ;
	v8.Texcoord = Vector2::Zero;

	v9.Position = Vector3(-1.0f, 1.0f, -1.0f) * scale;
	v9.Normal = -Vector3::UnitZ;
	v9.Texcoord = Vector2::UnitX;

	v10.Position = Vector3(1.0f, 1.0f, -1.0f) * scale;
	v10.Normal = -Vector3::UnitZ;
	v10.Texcoord = Vector2::One;

	v11.Position = Vector3(1.0f, -1.0f, -1.0f) * scale;
	v11.Normal = -Vector3::UnitZ;
	v11.Texcoord = Vector2::UnitY;

	// 뒷면
	Vertex& v12 = pOutDst->Vertices[12];
	Vertex& v13 = pOutDst->Vertices[13];
	Vertex& v14 = pOutDst->Vertices[14];
	Vertex& v15 = pOutDst->Vertices[15];

	v12.Position = Vector3(-1.0f, -1.0f, 1.0f) * scale;
	v12.Normal = Vector3::UnitZ;
	v12.Texcoord = Vector2::Zero;

	v13.Position = Vector3(1.0f, -1.0f, 1.0f) * scale;
	v13.Normal = Vector3::UnitZ;
	v13.Texcoord = Vector2::UnitX;

	v14.Position = Vector3::One * scale;
	v14.Normal = Vector3::UnitZ;
	v14.Texcoord = Vector2::One;

	v15.Position = Vector3(-1.0f, 1.0f, 1.0f) * scale;
	v15.Normal = Vector3::UnitZ;
	v15.Texcoord = Vector2::UnitY;

	// 왼쪽
	Vertex& v16 = pOutDst->Vertices[16];
	Vertex& v17 = pOutDst->Vertices[17];
	Vertex& v18 = pOutDst->Vertices[18];
	Vertex& v19 = pOutDst->Vertices[19];

	v16.Position = Vector3(-1.0f, -1.0f, 1.0f) * scale;
	v16.Normal = -Vector3::UnitX;
	v16.Texcoord = Vector2::Zero;

	v17.Position = Vector3(-1.0f, 1.0f, 1.0f) * scale;
	v17.Normal = -Vector3::UnitX;
	v17.Texcoord = Vector2::UnitX;

	v18.Position = Vector3(-1.0f, 1.0f, -1.0f) * scale;
	v18.Normal = -Vector3::UnitX;
	v18.Texcoord = Vector2::One;

	v19.Position = Vector3(-1.0f, -1.0f, -1.0f) * scale;
	v19.Normal = -Vector3::UnitX;
	v19.Texcoord = Vector2::UnitY;

	// 오른쪽
	Vertex& v20 = pOutDst->Vertices[20];
	Vertex& v21 = pOutDst->Vertices[21];
	Vertex& v22 = pOutDst->Vertices[22];
	Vertex& v23 = pOutDst->Vertices[23];

	v20.Position = Vector3(1.0f, -1.0f, 1.0f) * scale;
	v20.Normal = Vector3::UnitX;
	v20.Texcoord = Vector2::Zero;

	v21.Position = Vector3(1.0f, -1.0f, -1.0f) * scale;
	v21.Normal = Vector3::UnitX;
	v21.Texcoord = Vector2::UnitX;

	v22.Position = Vector3(1.0f, 1.0f, -1.0f) * scale;
	v22.Normal = Vector3::UnitX;
	v22.Texcoord = Vector2::One;

	v23.Position = Vector3::One * scale;
	v23.Normal = Vector3::UnitX;
	v23.Texcoord = Vector2::UnitY;

	pOutDst->Indices =
	{
		0,  1,  2,  0,  2,  3,  // 윗면
		4,  5,  6,  4,  6,  7,  // 아랫면
		8,  9,  10, 8,  10, 11, // 앞면
		12, 13, 14, 12, 14, 15, // 뒷면
		16, 17, 18, 16, 18, 19, // 왼쪽
		20, 21, 22, 20, 22, 23  // 오른쪽
	};
}

void MakeWireBox(MeshInfo* pOutDst, Vector3& center, Vector3& extents)
{
	// 상자를 와이어 프레임으로 그리는 용도.

	_ASSERT(pOutDst);

	pOutDst->Vertices.resize(8);

	// 앞면
	Vertex& v0 = pOutDst->Vertices[0];
	Vertex& v1 = pOutDst->Vertices[1];
	Vertex& v2 = pOutDst->Vertices[2];
	Vertex& v3 = pOutDst->Vertices[3];

	v0.Position = center + (-Vector3::One) * extents;
	v0.Normal = pOutDst->Vertices[0].Position - center;
	v0.Normal.Normalize();
	v0.Texcoord = Vector2::Zero;

	v1.Position = center + Vector3(-1.0f, 1.0f, -1.0f) * extents;
	v1.Normal = pOutDst->Vertices[1].Position - center;
	v1.Normal.Normalize();
	v1.Texcoord = Vector2::Zero;

	v2.Position = center + Vector3(1.0f, 1.0f, -1.0f) * extents;
	v2.Normal = pOutDst->Vertices[2].Position - center;
	v2.Normal.Normalize();
	v2.Normal.Normalize();

	v3.Position = center + Vector3(1.0f, -1.0f, -1.0f) * extents;
	v3.Normal = pOutDst->Vertices[3].Position - center;
	v3.Normal.Normalize();
	v3.Texcoord = Vector2::Zero;

	// 뒷면
	Vertex& v4 = pOutDst->Vertices[4];
	Vertex& v5 = pOutDst->Vertices[5];
	Vertex& v6 = pOutDst->Vertices[6];
	Vertex& v7 = pOutDst->Vertices[7];

	v4.Position = center + Vector3(-1.0f, -1.0f, 1.0f) * extents;
	v4.Normal = pOutDst->Vertices[4].Position - center;
	v4.Normal.Normalize();
	v4.Texcoord = Vector2::Zero;

	v5.Position = center + Vector3(-1.0f, 1.0f, 1.0f) * extents;
	v5.Normal = pOutDst->Vertices[5].Position - center;
	v5.Normal.Normalize();
	v5.Texcoord = Vector2::Zero;

	v6.Position = center + Vector3::One * extents;
	v6.Normal = pOutDst->Vertices[6].Position - center;
	v6.Normal.Normalize();
	v6.Texcoord = Vector2::Zero;

	v7.Position = center + Vector3(1.0f, -1.0f, 1.0f) * extents;
	v7.Normal = pOutDst->Vertices[7].Position - center;
	v7.Normal.Normalize();
	v7.Texcoord = Vector2::Zero;

	// Line list.
	pOutDst->Indices =
	{
		0, 1, 1, 2, 2, 3, 3, 0, // 앞면
		4, 5, 5, 6, 6, 7, 7, 4, // 뒷면
		0, 4, 1, 5, 2, 6, 3, 7  // 옆면
	};
}

void MakeWireSphere(MeshInfo* pOutDst, Vector3& center, float radius)
{
	_ASSERT(pOutDst);

	std::vector<Vertex>& vertices = pOutDst->Vertices;
	std::vector<UINT>& indices = pOutDst->Indices;

	const int NUM_POINTS = 30;
	const float D_THETA = DirectX::XM_2PI / (float)NUM_POINTS;

	// XY plane
	UINT offset = (UINT)vertices.size();
	Vector3 start = Vector3::UnitX;
	for (int i = 0; i < NUM_POINTS; ++i)
	{
		Vertex v;
		v.Position = center + Vector3::Transform(start, Matrix::CreateRotationZ(D_THETA * (float)i)) * radius;
		vertices.push_back(v);
		indices.push_back(i + offset);
		if (i != 0)
		{
			indices.push_back(i + offset);
		}
	}
	indices.push_back(offset);

	// YZ
	offset = (UINT)vertices.size();
	start = Vector3::UnitY;
	for (int i = 0; i < NUM_POINTS; ++i)
	{
		Vertex v;
		v.Position = center + Vector3::Transform(start, Matrix::CreateRotationX(D_THETA * (float)i)) * radius;
		vertices.push_back(v);
		indices.push_back(i + offset);
		if (i != 0)
		{
			indices.push_back(i + offset);
		}
	}
	indices.push_back(offset);

	// XZ
	offset = (UINT)vertices.size();
	start = Vector3::UnitX;
	for (int i = 0; i < NUM_POINTS; ++i)
	{
		Vertex v;
		v.Position = center + Vector3::Transform(start, Matrix::CreateRotationY(D_THETA * (float)i)) * radius;
		vertices.push_back(v);
		indices.push_back(i + offset);
		if (i != 0)
		{
			indices.push_back(i + offset);
		}
	}
	indices.push_back(offset);
}

void MakeCylinder(MeshInfo* pOutDst, float bottomRadius, float topRadius, float height, int numSlices)
{
	_ASSERT(pOutDst);

	// Texture 좌표계때문에 (NUM_SLICES + 1) x 2 개의 버텍스 사용.

	const float D_THETA = -DirectX::XM_2PI / (float)numSlices;

	std::vector<Vertex>& vertices = pOutDst->Vertices;
	std::vector<UINT>& indices = pOutDst->Indices;
	vertices.resize(numSlices * numSlices);
	indices.reserve(numSlices * 6);

	// 옆면의 바닥 버텍스들 (인덱스 0 이상 NUM_SLICES 미만).
	for (int i = 0; i <= numSlices; ++i)
	{
		Vertex& v = vertices[i];
		
		v.Position = Vector3::Transform(Vector3(bottomRadius, -0.5f * height, 0.0f), Matrix::CreateRotationY(D_THETA * (float)i));
		
		v.Normal = v.Position - Vector3(0.0f, -0.5f * height, 0.0f);
		v.Normal.Normalize();
		
		v.Texcoord = Vector2(float(i) / numSlices, 1.0f);
	}

	// 옆면의 맨 위 버텍스들 (인덱스 NUM_SLICES 이상 2 * NUM_SLICES 미만).
	for (int i = 0; i <= numSlices; ++i)
	{
		Vertex& v = vertices[numSlices + i];
		
		v.Position = Vector3::Transform(Vector3(topRadius, 0.5f * height, 0.0f), Matrix::CreateRotationY(D_THETA * (float)i));
		
		v.Normal = v.Position - Vector3(0.0f, 0.5f * height, 0.0f);
		v.Normal.Normalize();
		
		v.Texcoord = Vector2((float)i / numSlices, 0.0f);
	}

	for (int i = 0; i < numSlices; ++i)
	{
		indices.push_back(i);
		indices.push_back(i + numSlices + 1);
		indices.push_back(i + 1 + numSlices + 1);

		indices.push_back(i);
		indices.push_back(i + 1 + numSlices + 1);
		indices.push_back(i + 1);
	}
}

void MakeSphere(MeshInfo* pOutDst, float radius, int numSlices, int numStacks, Vector2 texScale)
{
	// 참고: OpenGL Sphere
	// http://www.songho.ca/opengl/gl_sphere.html
	// Texture 좌표계때문에 (NUM_SLICES + 1) 개의 버텍스 사용 (마지막에 닫아주는
	// 버텍스가 중복) Stack은 y 위쪽 방향으로 쌓아가는 방식.

	_ASSERT(pOutDst);

	const float D_THETA = -DirectX::XM_2PI / (float)numSlices;
	const float D_PHI = -DirectX::XM_PI / (float)numStacks;

	std::vector<Vertex>& vertices = pOutDst->Vertices;
	std::vector<UINT>& indices = pOutDst->Indices;
	vertices.resize((numStacks + 1) * (numSlices + 1));
	indices.reserve(numSlices * numStacks * 6);

	for (int j = 0; j <= numStacks; ++j)
	{
		// 스택에 쌓일 수록 시작점을 x-y 평면에서 회전 시켜서 위로 올리는 구조
		Vector3 stackStartPoint = Vector3::Transform(Vector3(0.0f, -radius, 0.0f), Matrix::CreateRotationZ(D_PHI * j));

		for (int i = 0; i <= numSlices; ++i)
		{
			Vertex& v = vertices[j * (numSlices + 1) + i];

			// 시작점을 x-z 평면에서 회전시키면서 원을 만드는 구조.
			v.Position = Vector3::Transform(stackStartPoint, Matrix::CreateRotationY(D_THETA * (float)i));

			v.Normal = v.Position; // 원점이 구의 중심.
			v.Normal.Normalize();

			v.Texcoord = Vector2((float)i / numSlices, 1.0f - (float)j / numStacks) * texScale;

			// Texcoord가 위로 갈수록 증가.
			Vector3 biTangent = Vector3::UnitY;
			Vector3 normalOrth = v.Normal - biTangent.Dot(v.Normal) * v.Normal;
			normalOrth.Normalize();

			v.Tangent = biTangent.Cross(normalOrth);
			v.Tangent.Normalize();
		}
	}

	for (int j = 0; j < numStacks; ++j)
	{
		const int OFFSET = (numSlices + 1) * j;

		for (int i = 0; i < numSlices; ++i)
		{
			indices.push_back(OFFSET + i);
			indices.push_back(OFFSET + i + numSlices + 1);
			indices.push_back(OFFSET + i + 1 + numSlices + 1);

			indices.push_back(OFFSET + i);
			indices.push_back(OFFSET + i + 1 + numSlices + 1);
			indices.push_back(OFFSET + i + 1);
		}
	}
}

void MakeTetrahedron(MeshInfo* pOutDst)
{
	// Regular Tetrahedron.
	// https://mathworld.wolfram.com/RegularTetrahedron.html

	_ASSERT(pOutDst);

	pOutDst->Vertices.resize(4);

	const float A = 1.0f;
	const float X = sqrt(3.0f) / 3.0f * A;
	const float D = sqrt(3.0f) / 6.0f * A; // = x / 2
	const float H = sqrt(6.0f) / 3.0f * A;

	Vector3 points[4] =
	{
		{ 0.0f, X, 0.0f },
		{ -0.5f * A, -D, 0.0f },
		{ 0.5f * A, -D, 0.0f },
		{ 0.0f, 0.0f, H }
	};
	Vector3 center;

	for (int i = 0; i < 4; ++i)
	{
		center += points[i];
	}
	center /= 4.0f;
	for (int i = 0; i < 4; ++i)
	{
		points[i] -= center;
	}

	for (int i = 0; i < 4; ++i)
	{
		Vertex& v = pOutDst->Vertices[i];
		v.Position = points[i];
		v.Normal = v.Position; // 중심이 원점.
		v.Normal.Normalize();
	}

	pOutDst->Indices = { 0, 1, 2, 3, 2, 1, 0, 3, 1, 0, 2, 3 };
}

void MakeIcosahedron(MeshInfo* pOutDst)
{
	// 등20면체.
	// https://mathworld.wolfram.com/Isohedron.html

	_ASSERT(pOutDst);

	const float X = 0.525731f;
	const float Z = 0.850651f;

	pOutDst->Vertices.resize(12);

	Vector3 pos[12] =
	{
		Vector3(-X, 0.0f, Z), Vector3(X, 0.0f, Z),   Vector3(-X, 0.0f, -Z),
		Vector3(X, 0.0f, -Z), Vector3(0.0f, Z, X),   Vector3(0.0f, Z, -X),
		Vector3(0.0f, -Z, X), Vector3(0.0f, -Z, -X), Vector3(Z, X, 0.0f),
		Vector3(-Z, X, 0.0f), Vector3(Z, -X, 0.0f),  Vector3(-Z, -X, 0.0f)
	};
	for (int i = 0; i < 12; ++i)
	{
		Vertex& v = pOutDst->Vertices[i];
		v.Position = pos[i];
		v.Normal = v.Position;
		v.Normal.Normalize();
	}

	pOutDst->Indices =
	{
		1,  4,  0, 4,  9, 0, 4, 5,  9, 8, 5, 4,  1,  8, 4,
		1,  10, 8, 10, 3, 8, 8, 3,  5, 3, 2, 5,  3,  7, 2,
		3,  10, 7, 10, 6, 7, 6, 11, 7, 6, 0, 11, 6,  1, 0,
		10, 1,  6, 11, 0, 9, 2, 11, 9, 5, 2, 9,  11, 2, 7
	};
}

void MakeTerrainTile(MeshInfo* pOutDst)
{
	_ASSERT(pOutDst);

	//MakeSquareGrid(pDst, 512, 512, 128, DirectX::SimpleMath::Vector2(512.0f));
	MakeSquareGrid(pOutDst, 10, 10, 256);

	/*srand((unsigned int)time(nullptr));
	for (SIZE_T i = 0, size = pDst->Vertices.size(); i < size; ++i)
	{
		DirectX::SimpleMath::Vector3& pos = pDst->Vertices[i].Position;
		pos.y = GetHeight(pos.x, pos.z);
		pos.y *= 2.0f;
	}*/
}

void SubdivideToSphere(MeshInfo* pOutDst, float radius, MeshInfo& meshData)
{
	using namespace DirectX;
	using DirectX::SimpleMath::Matrix;
	using DirectX::SimpleMath::Vector3;

	_ASSERT(pOutDst);

	// 원점이 중심이라고 가정.
	for (SIZE_T i = 0, size = meshData.Vertices.size(); i < size; ++i)
	{
		Vertex& v = meshData.Vertices[i];
		v.Position = v.Normal * radius;
	}

	// 구의 표면으로 옮기고 노멀과 texture 좌표 계산.
	auto ProjectVertex = [&](Vertex& v)
	{
		v.Normal = v.Position;
		v.Normal.Normalize();
		v.Position = v.Normal * radius;

		// 주의: 텍스춰가 이음매에서 깨집니다.
		// atan vs atan2
		// https://stackoverflow.com/questions/283406/what-is-the-difference-between-atan-and-atan2-in-c
		// const float theta = atan2f(v.Position.z, v.Position.x);
		// const float phi = acosf(v.Position.y / radius);
		// v.Texcoord.x = theta / XM_2PI;
		// v.Texcoord.y = phi / XM_PI;
	};

	/*auto UpdateFaceNormal = [](Vertex& v0, Vertex& v1, Vertex& v2)
	{
		Vector3 faceNormal = (v1.Position - v0.Position).Cross(v2.Position - v0.Position);
		faceNormal.Normalize();
		v0.Normal = faceNormal;
		v1.Normal = faceNormal;
		v2.Normal = faceNormal;
	};*/

	// 버텍스가 중복되는 구조로 구현.
	const SIZE_T TOTAL_INDICES = meshData.Indices.size();
	UINT count = 0;
	pOutDst->Vertices.reserve(12 * TOTAL_INDICES);
	pOutDst->Indices.reserve(12 * TOTAL_INDICES);

	for (SIZE_T i = 0; i < TOTAL_INDICES; i += 3)
	{
		UINT i0 = meshData.Indices[i];
		UINT i1 = meshData.Indices[i + 1];
		UINT i2 = meshData.Indices[i + 2];

		Vertex v0 = meshData.Vertices[i0];
		Vertex v1 = meshData.Vertices[i1];
		Vertex v2 = meshData.Vertices[i2];

		Vertex v3;
		v3.Position = (v0.Position + v2.Position) * 0.5f;
		v3.Texcoord = (v0.Texcoord + v2.Texcoord) * 0.5f;
		ProjectVertex(v3);

		Vertex v4;
		v4.Position = (v0.Position + v1.Position) * 0.5f;
		v4.Texcoord = (v0.Texcoord + v1.Texcoord) * 0.5f;
		ProjectVertex(v4);

		Vertex v5;
		v5.Position = (v1.Position + v2.Position) * 0.5f;
		v5.Texcoord = (v1.Texcoord + v2.Texcoord) * 0.5f;
		ProjectVertex(v5);

		// UpdateFaceNormal(v4, v1, v5);
		// UpdateFaceNormal(v0, v4, v3);
		// UpdateFaceNormal(v3, v4, v5);
		// UpdateFaceNormal(v3, v5, v2);

		pOutDst->Vertices.push_back(v4);
		pOutDst->Vertices.push_back(v1);
		pOutDst->Vertices.push_back(v5);

		pOutDst->Vertices.push_back(v0);
		pOutDst->Vertices.push_back(v4);
		pOutDst->Vertices.push_back(v3);

		pOutDst->Vertices.push_back(v3);
		pOutDst->Vertices.push_back(v4);
		pOutDst->Vertices.push_back(v5);

		pOutDst->Vertices.push_back(v3);
		pOutDst->Vertices.push_back(v5);
		pOutDst->Vertices.push_back(v2);

		for (UINT j = 0; j < 12; ++j)
		{
			pOutDst->Indices.push_back(j + count);
		}
		count += 12;
	}
}