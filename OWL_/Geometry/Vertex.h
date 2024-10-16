#pragma once

#include <directxtk/SimpleMath.h>

struct Vertex
{
	DirectX::SimpleMath::Vector3 Position;
	DirectX::SimpleMath::Vector3 Normal;
	DirectX::SimpleMath::Vector2 Texcoord;
	DirectX::SimpleMath::Vector3 Tangent;
};

struct SkinnedVertex
{
	DirectX::SimpleMath::Vector3 Position;
	DirectX::SimpleMath::Vector3 Normal;
	DirectX::SimpleMath::Vector2 Texcoord;
	DirectX::SimpleMath::Vector3 Tangent;

	float BlendWeights[8] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };  // BLENDWEIGHT0 and 1
	UINT8 BoneIndices[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // BLENDINDICES0 and 1
};

struct GrassVertex
{
	DirectX::SimpleMath::Vector3 Position;
	DirectX::SimpleMath::Vector3 Normal;
	DirectX::SimpleMath::Vector2 Texcoord;
};

// GrassVS, grassIL과 일관성이 있어야 합니다.
struct GrassInstance
{
	DirectX::SimpleMath::Matrix InstanceWorld; // <- Instance 단위의 Model to World 변환
	float WindStrength;
};