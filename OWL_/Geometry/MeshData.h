#pragma once

#pragma once

#include <vector>
#include "Vertex.h"

namespace Geometry
{
	struct MeshData
	{
		std::vector<struct Vertex> Vertices;
		std::vector<struct SkinnedVertex> SkinnedVertices;
		std::vector<uint32_t> Indices;
		std::wstring szAlbedoTextureFileName;
		std::wstring szEmissiveTextureFileName;
		std::wstring szNormalTextureFileName;
		std::wstring szHeightTextureFileName;
		std::wstring szAOTextureFileName; // Ambient Occlusion
		std::wstring szMetallicTextureFileName;
		std::wstring szRoughnessTextureFileName;
		std::wstring szOpacityTextureFileName;
	};
}

#define INIT_MESH_DATA							 \
	{											 \
		{}, {}, {},								 \
		L"",  L"", L"", L"", L"", L"", L"", L"", \
	}

static void ReleaseMeshData(struct Geometry::MeshData** ppMeshData)
{
	_ASSERT(*ppMeshData);

	(*ppMeshData)->Vertices.clear();
	(*ppMeshData)->SkinnedVertices.clear();
	(*ppMeshData)->Indices.clear();

	free(*ppMeshData);
	*ppMeshData = nullptr;
}
