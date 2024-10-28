#include "../Common.h"
#include "TerrainGenerator.h"

using DirectX::SimpleMath::Vector3;

void TerrainGenerator::GenerateHeight(std::vector<Vertex>* pVertices)
{
	_ASSERT(pVertices);

	for (UINT64 i = 0, size = pVertices->size(); i < size; ++i)
	{
		Vector3& pos = (*pVertices)[i].Position;

		float amplitude = 1.0f;
		float frequency = 1.0f;
		float noiseHeight = 0.0f;

		for (int i = 0; i < m_Octaves; ++i)
		{
			float xCoord;
		}
	}
}
