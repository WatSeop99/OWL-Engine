#pragma once

#pragma once

#include <vector>
#include "Vertex.h"

struct MeshInfo
{
	std::vector<Vertex> Vertices;
	std::vector<SkinnedVertex> SkinnedVertices;
	std::vector<UINT> Indices;
	std::wstring szAlbedoTextureFileName;
	std::wstring szEmissiveTextureFileName;
	std::wstring szNormalTextureFileName;
	std::wstring szHeightTextureFileName;
	std::wstring szAOTextureFileName; // Ambient Occlusion
	std::wstring szMetallicTextureFileName;
	std::wstring szRoughnessTextureFileName;
	std::wstring szOpacityTextureFileName;
};
