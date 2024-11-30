#pragma once

#include "Model.h"

ALIGN(16) struct TerrainConstants
{
	float Scale;
	int Octaves;
	float Persistance;
	float Lacunarity;
	float MinHeight;
	float MaxHeight;
};

class Renderer;
class ConstantBuffer;
class Texture;
class StructuredBuffer;

class Terrain final : public Model
{
public:
	Terrain() = default;
	~Terrain() { Cleanup(); }

	void Initialize(Renderer* pRenderer);

	void Update(Renderer* pRenderer);

	void Render() override;

	void Cleanup();

public:
	float NoiseScale = 25.0f;
	int Octaves = 1;
	float Persistance = 1.0f;
	float Lacunarity = 1.0f;
	DirectX::SimpleMath::Vector2 Offset = DirectX::SimpleMath::Vector2(51.11, 0.0f);
	//DirectX::SimpleMath::Vector2 Offset;

private:
	static int ms_TerrainCount;

	ConstantBuffer* m_pTerrainConstantBuffer = nullptr;
	Texture* m_pHeightMap = nullptr;
	Texture* m_pColorMap = nullptr;
	StructuredBuffer* m_pOctaveOffset = nullptr;

	UINT m_ChunkSize = 256;
};
