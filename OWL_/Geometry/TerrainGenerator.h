#pragma once

#include <limits.h>
#include "Vertex.h"

class TerrainGenerator
{
public:
	TerrainGenerator() = default;
	~TerrainGenerator() = default;

	void GenerateHeight(std::vector<Vertex>* pVertices);

	inline void SetMaxHeight(float maxHeight) { m_MaxHeight = maxHeight; }
	inline void SetMinHeight(float minHeight) { m_MinHeight = minHeight; }

private:
	float m_MaxHeight = FLT_MAX;
	float m_MinHeight = FLT_MIN;
	float m_Octaves;
};

