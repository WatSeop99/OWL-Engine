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

	Normalize(Vector3(0.0f), 1.0f, modelLoader.pMeshInfos, modelLoader.AnimData);
	dst = modelLoader.pMeshInfos;

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

	Normalize(Vector3(0.0f), 1.0f, modelLoader.pMeshInfos, modelLoader.AnimData);
	dst = { modelLoader.pMeshInfos, modelLoader.AnimData };

LB_RET:
	return hr;
}

void Normalize(const Vector3& CENTER, const float LONGEST_LENGTH, std::vector<MeshInfo>& meshes, AnimationData& animData)
{
	// 모델의 중심을 원점으로 옮기고 크기를 [-1,1]^3으로 스케일 -> 박스 형태로.

	// Normalize vertices
	Vector3 vMin(1000.0f, 1000.0f, 1000.0f);
	Vector3 vMax(-1000.0f, -1000.0f, -1000.0f);
	for (UINT64 i = 0, totalMesh = meshes.size(); i < totalMesh; ++i)
	{
		MeshInfo& curMesh = meshes[i];
		for (UINT64 j = 0, vertSize = curMesh.Vertices.size(); j < vertSize; ++j)
		{
			Vertex& v = curMesh.Vertices[j];
			vMin = Min(vMin, v.Position);
			vMax = Max(vMax, v.Position);
		}
	}

	Vector3 delta = vMax - vMin;
	float scale = LONGEST_LENGTH / DirectX::XMMax(DirectX::XMMax(delta.x, delta.y), delta.z);
	Vector3 translation = -(vMin + vMax) * 0.5f + CENTER;

	for (UINT64 i = 0, totalMesh = meshes.size(); i < totalMesh; ++i)
	{
		MeshInfo& curMesh = meshes[i];
		for (UINT64 j = 0, vertSize = curMesh.Vertices.size(); j < vertSize; ++j)
		{
			Vertex& v = curMesh.Vertices[j];
			v.Position = (v.Position + translation) * scale;
		}
		for (UINT64 j = 0, skinnedVertSize = curMesh.SkinnedVertices.size(); j < skinnedVertSize; ++j)
		{
			SkinnedVertex& v = curMesh.SkinnedVertices[j];
			v.Position = (v.Position + translation) * scale;
		}
	}

	// 애니메이션 데이터 보정에 사용.
	animData.DefaultTransform = Matrix::CreateTranslation(translation) * Matrix::CreateScale(scale);
}

void MakeSquare(MeshInfo* pDst, const float SCALE, const Vector2 TEX_SCALE)
{
	// Texture Coordinates (Direct3D 9)
	// https://learn.microsoft.com/en-us/windows/win32/direct3d9/texture-coordinates

	_ASSERT(pDst);

	pDst->Vertices.resize(4);

	Vertex& v0 = pDst->Vertices[0];
	Vertex& v1 = pDst->Vertices[1];
	Vertex& v2 = pDst->Vertices[2];
	Vertex& v3 = pDst->Vertices[3];

	v0.Position = Vector3(-1.0f, 1.0f, 0.0f) * SCALE;
	v0.Normal = Vector3(0.0f, 0.0f, -1.0f);
	v0.Texcoord = Vector2(0.0f, 0.0f) * TEX_SCALE;
	v0.Tangent = Vector3(1.0f, 0.0f, 0.0f);

	v1.Position = Vector3(1.0f, 1.0f, 0.0f) * SCALE;
	v1.Normal = Vector3(0.0f, 0.0f, -1.0f);
	v1.Texcoord = Vector2(1.0f, 0.0f) * TEX_SCALE;
	v1.Tangent = Vector3(1.0f, 0.0f, 0.0f);

	v2.Position = Vector3(1.0f, -1.0f, 0.0f) * SCALE;
	v2.Normal = Vector3(0.0f, 0.0f, -1.0f);
	v2.Texcoord = Vector2(1.0f, 1.0f) * TEX_SCALE;
	v2.Tangent = Vector3(1.0f, 0.0f, 0.0f);

	v3.Position = Vector3(-1.0f, -1.0f, 0.0f) * SCALE;
	v3.Normal = Vector3(0.0f, 0.0f, -1.0f);
	v3.Texcoord = Vector2(0.0f, 1.0f) * TEX_SCALE;
	v3.Tangent = Vector3(1.0f, 0.0f, 0.0f);

	pDst->Indices = { 0, 1, 2, 0, 2, 3, };
}

void MakeSquareGrid(MeshInfo* pDst, const int NUM_SLICES, const int NUM_STACKS, const float SCALE, const Vector2 TEX_SCALE)
{
	_ASSERT(pDst);

	pDst->Vertices.resize((NUM_STACKS + 1) * (NUM_SLICES + 1));
	pDst->Indices.reserve(NUM_STACKS * NUM_SLICES * 6);

	float dx = 2.0f / NUM_SLICES;
	float dy = 2.0f / NUM_STACKS;

	float y = 1.0f;
	for (int j = 0; j < NUM_STACKS + 1; ++j)
	{
		float x = -1.0f;
		for (int i = 0; i < NUM_SLICES + 1; ++i)
		{
			Vertex& v = pDst->Vertices[j * (NUM_SLICES + 1) + i];
			v.Position = Vector3(x, y, 0.0f) * SCALE;
			v.Normal = Vector3(0.0f, 0.0f, -1.0f);
			v.Texcoord = Vector2(x + 1.0f, y + 1.0f) * 0.5f * TEX_SCALE;
			v.Tangent = Vector3(1.0f, 0.0f, 0.0f);

			x += dx;
		}
		y -= dy;
	}

	for (int j = 0; j < NUM_STACKS; ++j)
	{
		for (int i = 0; i < NUM_SLICES; ++i)
		{
			pDst->Indices.push_back((NUM_SLICES + 1) * j + i);
			pDst->Indices.push_back((NUM_SLICES + 1) * j + i + 1);
			pDst->Indices.push_back((NUM_SLICES + 1) * (j + 1) + i);

			pDst->Indices.push_back((NUM_SLICES + 1) * (j + 1) + i);
			pDst->Indices.push_back((NUM_SLICES + 1) * j + i + 1);
			pDst->Indices.push_back((NUM_SLICES + 1) * (j + 1) + i + 1);
		}
	}
}

void MakeGrass(MeshInfo* pDst)
{
	_ASSERT(pDst);

	MakeSquareGrid(pDst, 1, 4);

	for (UINT64 i = 0, size = pDst->Vertices.size(); i < size; ++i)
	{
		Vertex& v = pDst->Vertices[i];

		// 적당히 가늘게 조절.
		v.Position.x *= 0.02f;

		// Y범위를 0~1로 조절.
		v.Position.y = v.Position.y * 0.5f + 0.5f;
	}

	// 맨 위를 뾰족하게 만들기 위해 삼각형 하나와 정점 하나 삭제.
	pDst->Indices.erase(pDst->Indices.begin(), pDst->Indices.begin() + 3);
	for (UINT64 i = 0, size = pDst->Indices.size(); i < size; ++i)
	{
		pDst->Indices[i] -= 1;
	}
	pDst->Vertices.erase(pDst->Vertices.begin());
	pDst->Vertices[0].Position.x = 0.0f;
	pDst->Vertices[0].Texcoord.x = 0.5f;
}

void MakeBox(MeshInfo* pDst, const float SCALE)
{
	_ASSERT(pDst);

	pDst->Vertices.resize(24);

	// 윗면
	Vertex& v0 = pDst->Vertices[0];
	Vertex& v1 = pDst->Vertices[1];
	Vertex& v2 = pDst->Vertices[2];
	Vertex& v3 = pDst->Vertices[3];

	v0.Position = Vector3(-1.0f, 1.0f, -1.0f) * SCALE;
	v0.Normal = Vector3(0.0f, 1.0f, 0.0f);
	v0.Texcoord = Vector2(0.0f, 0.0f);

	v1.Position = Vector3(-1.0f, 1.0f, 1.0f) * SCALE;
	v1.Normal = Vector3(0.0f, 1.0f, 0.0f);
	v1.Texcoord = Vector2(1.0f, 0.0f);

	v2.Position = Vector3(1.0f, 1.0f, 1.0f) * SCALE;
	v2.Normal = Vector3(0.0f, 1.0f, 0.0f);
	v2.Texcoord = Vector2(1.0f, 1.0f);

	v3.Position = Vector3(1.0f, 1.0f, -1.0f) * SCALE;
	v3.Normal = Vector3(0.0f, 1.0f, 0.0f);
	v3.Texcoord = Vector2(0.0f, 1.0f);

	// 아랫면
	Vertex& v4 = pDst->Vertices[4];
	Vertex& v5 = pDst->Vertices[5];
	Vertex& v6 = pDst->Vertices[6];
	Vertex& v7 = pDst->Vertices[7];

	v4.Position = Vector3(-1.0f, -1.0f, -1.0f) * SCALE;
	v4.Normal = Vector3(0.0f, -1.0f, 0.0f);
	v4.Texcoord = Vector2(0.0f, 0.0f);

	v5.Position = Vector3(1.0f, -1.0f, -1.0f) * SCALE;
	v5.Normal = Vector3(0.0f, -1.0f, 0.0f);
	v5.Texcoord = Vector2(1.0f, 0.0f);

	v6.Position = Vector3(1.0f, -1.0f, 1.0f) * SCALE;
	v6.Normal = Vector3(0.0f, -1.0f, 0.0f);
	v6.Texcoord = Vector2(1.0f, 1.0f);

	v7.Position = Vector3(-1.0f, -1.0f, 1.0f) * SCALE;
	v7.Normal = Vector3(0.0f, -1.0f, 0.0f);
	v7.Texcoord = Vector2(0.0f, 1.0f);

	// 앞면
	Vertex& v8 = pDst->Vertices[8];
	Vertex& v9 = pDst->Vertices[9];
	Vertex& v10 = pDst->Vertices[10];
	Vertex& v11 = pDst->Vertices[11];

	v8.Position = Vector3(-1.0f, -1.0f, -1.0f) * SCALE;
	v8.Normal = Vector3(0.0f, 0.0f, -1.0f);
	v8.Texcoord = Vector2(0.0f, 0.0f);

	v9.Position = Vector3(-1.0f, 1.0f, -1.0f) * SCALE;
	v9.Normal = Vector3(0.0f, 0.0f, -1.0f);
	v9.Texcoord = Vector2(1.0f, 0.0f);

	v10.Position = Vector3(1.0f, 1.0f, -1.0f) * SCALE;
	v10.Normal = Vector3(0.0f, 0.0f, -1.0f);
	v10.Texcoord = Vector2(1.0f, 1.0f);

	v11.Position = Vector3(1.0f, -1.0f, -1.0f) * SCALE;
	v11.Normal = Vector3(0.0f, 0.0f, -1.0f);
	v11.Texcoord = Vector2(0.0f, 1.0f);

	// 뒷면
	Vertex& v12 = pDst->Vertices[12];
	Vertex& v13 = pDst->Vertices[13];
	Vertex& v14 = pDst->Vertices[14];
	Vertex& v15 = pDst->Vertices[15];

	v12.Position = Vector3(-1.0f, -1.0f, 1.0f) * SCALE;
	v12.Normal = Vector3(0.0f, 0.0f, 1.0f);
	v12.Texcoord = Vector2(0.0f, 0.0f);

	v13.Position = Vector3(1.0f, -1.0f, 1.0f) * SCALE;
	v13.Normal = Vector3(0.0f, 0.0f, 1.0f);
	v13.Texcoord = Vector2(1.0f, 0.0f);

	v14.Position = Vector3(1.0f, 1.0f, 1.0f) * SCALE;
	v14.Normal = Vector3(0.0f, 0.0f, 1.0f);
	v14.Texcoord = Vector2(1.0f, 1.0f);

	v15.Position = Vector3(-1.0f, 1.0f, 1.0f) * SCALE;
	v15.Normal = Vector3(0.0f, 0.0f, 1.0f);
	v15.Texcoord = Vector2(0.0f, 1.0f);

	// 왼쪽
	Vertex& v16 = pDst->Vertices[16];
	Vertex& v17 = pDst->Vertices[17];
	Vertex& v18 = pDst->Vertices[18];
	Vertex& v19 = pDst->Vertices[19];

	v16.Position = Vector3(-1.0f, -1.0f, 1.0f) * SCALE;
	v16.Normal = Vector3(-1.0f, 0.0f, 0.0f);
	v16.Texcoord = Vector2(0.0f, 0.0f);

	v17.Position = Vector3(-1.0f, 1.0f, 1.0f) * SCALE;
	v17.Normal = Vector3(-1.0f, 0.0f, 0.0f);
	v17.Texcoord = Vector2(1.0f, 0.0f);

	v18.Position = Vector3(-1.0f, 1.0f, -1.0f) * SCALE;
	v18.Normal = Vector3(-1.0f, 0.0f, 0.0f);
	v18.Texcoord = Vector2(1.0f, 1.0f);

	v19.Position = Vector3(-1.0f, -1.0f, -1.0f) * SCALE;
	v19.Normal = Vector3(-1.0f, 0.0f, 0.0f);
	v19.Texcoord = Vector2(0.0f, 1.0f);

	// 오른쪽
	Vertex& v20 = pDst->Vertices[20];
	Vertex& v21 = pDst->Vertices[21];
	Vertex& v22 = pDst->Vertices[22];
	Vertex& v23 = pDst->Vertices[23];

	v20.Position = Vector3(1.0f, -1.0f, 1.0f) * SCALE;
	v20.Normal = Vector3(1.0f, 0.0f, 0.0f);
	v20.Texcoord = Vector2(0.0f, 0.0f);

	v21.Position = Vector3(1.0f, -1.0f, -1.0f) * SCALE;
	v21.Normal = Vector3(1.0f, 0.0f, 0.0f);
	v21.Texcoord = Vector2(1.0f, 0.0f);

	v22.Position = Vector3(1.0f, 1.0f, -1.0f) * SCALE;
	v22.Normal = Vector3(1.0f, 0.0f, 0.0f);
	v22.Texcoord = Vector2(1.0f, 1.0f);

	v23.Position = Vector3(1.0f, 1.0f, 1.0f) * SCALE;
	v23.Normal = Vector3(1.0f, 0.0f, 0.0f);
	v23.Texcoord = Vector2(0.0f, 1.0f);

	pDst->Indices =
	{
		0,  1,  2,  0,  2,  3,  // 윗면
		4,  5,  6,  4,  6,  7,  // 아랫면
		8,  9,  10, 8,  10, 11, // 앞면
		12, 13, 14, 12, 14, 15, // 뒷면
		16, 17, 18, 16, 18, 19, // 왼쪽
		20, 21, 22, 20, 22, 23  // 오른쪽
	};
}

void MakeWireBox(MeshInfo* pDst, const Vector3& CENTER, const Vector3& EXTENTS)
{
	// 상자를 와이어 프레임으로 그리는 용도.

	_ASSERT(pDst);

	pDst->Vertices.resize(8);

	// 앞면
	Vertex& v0 = pDst->Vertices[0];
	Vertex& v1 = pDst->Vertices[1];
	Vertex& v2 = pDst->Vertices[2];
	Vertex& v3 = pDst->Vertices[3];

	v0.Position = CENTER + Vector3(-1.0f, -1.0f, -1.0f) * EXTENTS;
	v0.Normal = pDst->Vertices[0].Position - CENTER;
	v0.Normal.Normalize();
	v0.Texcoord = Vector2(0.0f);

	v1.Position = CENTER + Vector3(-1.0f, 1.0f, -1.0f) * EXTENTS;
	v1.Normal = pDst->Vertices[1].Position - CENTER;
	v1.Normal.Normalize();
	v1.Texcoord = Vector2(0.0f);

	v2.Position = CENTER + Vector3(1.0f, 1.0f, -1.0f) * EXTENTS;
	v2.Normal = pDst->Vertices[2].Position - CENTER;
	v2.Normal.Normalize();
	v2.Normal.Normalize();

	v3.Position = CENTER + Vector3(1.0f, -1.0f, -1.0f) * EXTENTS;
	v3.Normal = pDst->Vertices[3].Position - CENTER;
	v3.Normal.Normalize();
	v3.Texcoord = Vector2(0.0f);

	// 뒷면
	Vertex& v4 = pDst->Vertices[4];
	Vertex& v5 = pDst->Vertices[5];
	Vertex& v6 = pDst->Vertices[6];
	Vertex& v7 = pDst->Vertices[7];

	v4.Position = CENTER + Vector3(-1.0f, -1.0f, 1.0f) * EXTENTS;
	v4.Normal = pDst->Vertices[4].Position - CENTER;
	v4.Normal.Normalize();
	v4.Texcoord = Vector2(0.0f);

	v5.Position = CENTER + Vector3(-1.0f, 1.0f, 1.0f) * EXTENTS;
	v5.Normal = pDst->Vertices[5].Position - CENTER;
	v5.Normal.Normalize();
	v5.Texcoord = Vector2(0.0f);

	v6.Position = CENTER + Vector3(1.0f, 1.0f, 1.0f) * EXTENTS;
	v6.Normal = pDst->Vertices[6].Position - CENTER;
	v6.Normal.Normalize();
	v6.Texcoord = Vector2(0.0f);

	v7.Position = CENTER + Vector3(1.0f, -1.0f, 1.0f) * EXTENTS;
	v7.Normal = pDst->Vertices[7].Position - CENTER;
	v7.Normal.Normalize();
	v7.Texcoord = Vector2(0.0f);

	// Line list.
	pDst->Indices =
	{
		0, 1, 1, 2, 2, 3, 3, 0, // 앞면
		4, 5, 5, 6, 6, 7, 7, 4, // 뒷면
		0, 4, 1, 5, 2, 6, 3, 7  // 옆면
	};
}

void MakeWireSphere(MeshInfo* pDst, const Vector3& CENTER, const float RADIUS)
{
	_ASSERT(pDst);

	std::vector<Vertex>& vertices = pDst->Vertices;
	std::vector<UINT>& indices = pDst->Indices;

	const int NUM_POINTS = 30;
	const float D_THETA = DirectX::XM_2PI / (float)NUM_POINTS;

	// XY plane
	UINT offset = (UINT)(vertices.size());
	Vector3 start = Vector3::UnitX;
	for (int i = 0; i < NUM_POINTS; ++i)
	{
		Vertex v;
		v.Position = CENTER + Vector3::Transform(start, Matrix::CreateRotationZ(D_THETA * (float)i)) * RADIUS;
		vertices.push_back(v);
		indices.push_back(i + offset);
		if (i != 0)
		{
			indices.push_back(i + offset);
		}
	}
	indices.push_back(offset);

	// YZ
	offset = (UINT)(vertices.size());
	start = Vector3::UnitY;
	for (int i = 0; i < NUM_POINTS; ++i)
	{
		Vertex v;
		v.Position = CENTER + Vector3::Transform(start, Matrix::CreateRotationX(D_THETA * (float)i)) * RADIUS;
		vertices.push_back(v);
		indices.push_back(i + offset);
		if (i != 0)
		{
			indices.push_back(i + offset);
		}
	}
	indices.push_back(offset);

	// XZ
	offset = (UINT)(vertices.size());
	start = Vector3::UnitX;
	for (int i = 0; i < NUM_POINTS; ++i)
	{
		Vertex v;
		v.Position = CENTER + Vector3::Transform(start, Matrix::CreateRotationY(D_THETA * (float)i)) * RADIUS;
		vertices.push_back(v);
		indices.push_back(i + offset);
		if (i != 0)
		{
			indices.push_back(i + offset);
		}
	}
	indices.push_back(offset);
}

void MakeCylinder(MeshInfo* pDst, const float BOTTOM_RADIUS, const float TOP_RADIUS, const float HEIGHT, const int NUM_SLICES)
{
	_ASSERT(pDst);

	// Texture 좌표계때문에 (NUM_SLICES + 1) x 2 개의 버텍스 사용.

	const float D_THETA = -DirectX::XM_2PI / (float)NUM_SLICES;

	std::vector<Vertex>& vertices = pDst->Vertices;
	std::vector<UINT>& indices = pDst->Indices;
	vertices.resize(NUM_SLICES * NUM_SLICES);
	indices.reserve(NUM_SLICES * 6);

	// 옆면의 바닥 버텍스들 (인덱스 0 이상 NUM_SLICES 미만).
	for (int i = 0; i <= NUM_SLICES; ++i)
	{
		Vertex& v = vertices[i];
		
		v.Position = Vector3::Transform(Vector3(BOTTOM_RADIUS, -0.5f * HEIGHT, 0.0f), Matrix::CreateRotationY(D_THETA * (float)i));
		
		v.Normal = v.Position - Vector3(0.0f, -0.5f * HEIGHT, 0.0f);
		v.Normal.Normalize();
		
		v.Texcoord = Vector2(float(i) / NUM_SLICES, 1.0f);
	}

	// 옆면의 맨 위 버텍스들 (인덱스 NUM_SLICES 이상 2 * NUM_SLICES 미만).
	for (int i = 0; i <= NUM_SLICES; ++i)
	{
		Vertex& v = vertices[NUM_SLICES + i];
		
		v.Position = Vector3::Transform(Vector3(TOP_RADIUS, 0.5f * HEIGHT, 0.0f), Matrix::CreateRotationY(D_THETA * (float)i));
		
		v.Normal = v.Position - Vector3(0.0f, 0.5f * HEIGHT, 0.0f);
		v.Normal.Normalize();
		
		v.Texcoord = Vector2((float)i / NUM_SLICES, 0.0f);
	}

	for (int i = 0; i < NUM_SLICES; ++i)
	{
		indices.push_back(i);
		indices.push_back(i + NUM_SLICES + 1);
		indices.push_back(i + 1 + NUM_SLICES + 1);

		indices.push_back(i);
		indices.push_back(i + 1 + NUM_SLICES + 1);
		indices.push_back(i + 1);
	}
}

void MakeSphere(MeshInfo* pDst, const float RADIUS, const int NUM_SLICES, const int NUM_STACKS, const Vector2 TEX_SCALE)
{
	// 참고: OpenGL Sphere
	// http://www.songho.ca/opengl/gl_sphere.html
	// Texture 좌표계때문에 (NUM_SLICES + 1) 개의 버텍스 사용 (마지막에 닫아주는
	// 버텍스가 중복) Stack은 y 위쪽 방향으로 쌓아가는 방식.

	_ASSERT(pDst);

	const float D_THETA = -DirectX::XM_2PI / (float)NUM_SLICES;
	const float D_PHI = -DirectX::XM_PI / (float)NUM_STACKS;

	std::vector<Vertex>& vertices = pDst->Vertices;
	std::vector<UINT>& indices = pDst->Indices;
	vertices.resize((NUM_STACKS + 1) * (NUM_SLICES + 1));
	indices.reserve(NUM_SLICES * NUM_STACKS * 6);

	for (int j = 0; j <= NUM_STACKS; ++j)
	{
		// 스택에 쌓일 수록 시작점을 x-y 평면에서 회전 시켜서 위로 올리는 구조
		Vector3 stackStartPoint = Vector3::Transform(Vector3(0.0f, -RADIUS, 0.0f), Matrix::CreateRotationZ(D_PHI * j));

		for (int i = 0; i <= NUM_SLICES; ++i)
		{
			Vertex& v = vertices[j * (NUM_SLICES + 1) + i];

			// 시작점을 x-z 평면에서 회전시키면서 원을 만드는 구조.
			v.Position = Vector3::Transform(stackStartPoint, Matrix::CreateRotationY(D_THETA * float(i)));

			v.Normal = v.Position; // 원점이 구의 중심.
			v.Normal.Normalize();

			v.Texcoord = Vector2(float(i) / NUM_SLICES, 1.0f - float(j) / NUM_STACKS) * TEX_SCALE;

			// Texcoord가 위로 갈수록 증가.
			Vector3 biTangent = Vector3::UnitY;
			Vector3 normalOrth = v.Normal - biTangent.Dot(v.Normal) * v.Normal;
			normalOrth.Normalize();

			v.Tangent = biTangent.Cross(normalOrth);
			v.Tangent.Normalize();
		}
	}

	for (int j = 0; j < NUM_STACKS; ++j)
	{
		const int OFFSET = (NUM_SLICES + 1) * j;

		for (int i = 0; i < NUM_SLICES; ++i)
		{
			indices.push_back(OFFSET + i);
			indices.push_back(OFFSET + i + NUM_SLICES + 1);
			indices.push_back(OFFSET + i + 1 + NUM_SLICES + 1);

			indices.push_back(OFFSET + i);
			indices.push_back(OFFSET + i + 1 + NUM_SLICES + 1);
			indices.push_back(OFFSET + i + 1);
		}
	}
}

void MakeTetrahedron(MeshInfo* pDst)
{
	// Regular Tetrahedron.
	// https://mathworld.wolfram.com/RegularTetrahedron.html

	_ASSERT(pDst);

	pDst->Vertices.resize(4);

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
		Vertex& v = pDst->Vertices[i];
		v.Position = points[i];
		v.Normal = v.Position; // 중심이 원점.
		v.Normal.Normalize();
	}

	pDst->Indices = { 0, 1, 2, 3, 2, 1, 0, 3, 1, 0, 2, 3 };
}

void MakeIcosahedron(MeshInfo* pDst)
{
	// 등20면체.
	// https://mathworld.wolfram.com/Isohedron.html

	_ASSERT(pDst);

	const float X = 0.525731f;
	const float Z = 0.850651f;

	pDst->Vertices.resize(12);

	Vector3 pos[12] =
	{
		Vector3(-X, 0.0f, Z), Vector3(X, 0.0f, Z),   Vector3(-X, 0.0f, -Z),
		Vector3(X, 0.0f, -Z), Vector3(0.0f, Z, X),   Vector3(0.0f, Z, -X),
		Vector3(0.0f, -Z, X), Vector3(0.0f, -Z, -X), Vector3(Z, X, 0.0f),
		Vector3(-Z, X, 0.0f), Vector3(Z, -X, 0.0f),  Vector3(-Z, -X, 0.0f)
	};
	for (int i = 0; i < 12; ++i)
	{
		Vertex& v = pDst->Vertices[i];
		v.Position = pos[i];
		v.Normal = v.Position;
		v.Normal.Normalize();
	}

	pDst->Indices =
	{
		1,  4,  0, 4,  9, 0, 4, 5,  9, 8, 5, 4,  1,  8, 4,
		1,  10, 8, 10, 3, 8, 8, 3,  5, 3, 2, 5,  3,  7, 2,
		3,  10, 7, 10, 6, 7, 6, 11, 7, 6, 0, 11, 6,  1, 0,
		10, 1,  6, 11, 0, 9, 2, 11, 9, 5, 2, 9,  11, 2, 7
	};
}

void SubdivideToSphere(MeshInfo* pDst, const float RADIUS, MeshInfo& meshData)
{
	using namespace DirectX;
	using DirectX::SimpleMath::Matrix;
	using DirectX::SimpleMath::Vector3;

	_ASSERT(pDst);

	// 원점이 중심이라고 가정.
	for (UINT64 i = 0, size = meshData.Vertices.size(); i < size; ++i)
	{
		Vertex& v = meshData.Vertices[i];
		v.Position = v.Normal * RADIUS;
	}

	// 구의 표면으로 옮기고 노멀과 texture 좌표 계산.
	auto ProjectVertex = [&](Vertex& v)
	{
		v.Normal = v.Position;
		v.Normal.Normalize();
		v.Position = v.Normal * RADIUS;

		// 주의: 텍스춰가 이음매에서 깨집니다.
		// atan vs atan2
		// https://stackoverflow.com/questions/283406/what-is-the-difference-between-atan-and-atan2-in-c
		// const float theta = atan2f(v.Position.z, v.Position.x);
		// const float phi = acosf(v.Position.y / radius);
		// v.Texcoord.x = theta / XM_2PI;
		// v.Texcoord.y = phi / XM_PI;
	};

	auto UpdateFaceNormal = [](Vertex& v0, Vertex& v1, Vertex& v2)
	{
		Vector3 faceNormal = (v1.Position - v0.Position).Cross(v2.Position - v0.Position);
		faceNormal.Normalize();
		v0.Normal = faceNormal;
		v1.Normal = faceNormal;
		v2.Normal = faceNormal;
	};

	// 버텍스가 중복되는 구조로 구현.
	const UINT64 TOTAL_INDICES = meshData.Indices.size();
	UINT count = 0;
	pDst->Vertices.reserve(12 * TOTAL_INDICES);
	pDst->Indices.reserve(12 * TOTAL_INDICES);

	for (UINT64 i = 0; i < TOTAL_INDICES; i += 3)
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

		pDst->Vertices.push_back(v4);
		pDst->Vertices.push_back(v1);
		pDst->Vertices.push_back(v5);

		pDst->Vertices.push_back(v0);
		pDst->Vertices.push_back(v4);
		pDst->Vertices.push_back(v3);

		pDst->Vertices.push_back(v3);
		pDst->Vertices.push_back(v4);
		pDst->Vertices.push_back(v5);

		pDst->Vertices.push_back(v3);
		pDst->Vertices.push_back(v5);
		pDst->Vertices.push_back(v2);

		for (UINT j = 0; j < 12; ++j)
		{
			pDst->Indices.push_back(j + count);
		}
		count += 12;
	}
}