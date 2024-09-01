#pragma once

#include <directxtk/SimpleMath.h>

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Quaternion;
using DirectX::SimpleMath::Vector3;

struct AnimationClip
{
	class Key
	{
	public:
		Key() = default;
		~Key() = default;

		Matrix GetTransform();

	public:
		Vector3 Position;
		Vector3 Scale = Vector3(1.0f);
		Quaternion Rotation;
	};

	std::string Name;					 // Name of this animation clip.
	std::vector<std::vector<Key>> Keys; // Keys[boneIDX][frameIDX].
	int NumChannels;					 // Number of bones.
	double Duration;					 // Duration of animation in ticks.
	double TicksPerSec;					 // Frames per second.
};

class AnimationData
{
public:
	AnimationData() = default;
	~AnimationData() = default;

	void Update(const int clipID, const int frame);

	inline Matrix Get(const int CLIP_ID, const int BONE_ID, const int FRAME)
	{
		// DefaultTransform은 모델을 읽어들일때 GeometryGenerator::Normalize()에서 계산. 
		// DefaultTransform.Invert() * OffsetMatrices[BONE_ID]를 미리 계산해서 합치고 
		// DefaultTransform * RootTransform을 미리 계산해놓을 수 있음.
		return (DefaultTransform.Invert() * OffsetMatrices[BONE_ID] * BoneTransforms[BONE_ID] * DefaultTransform);
	}

public:
	std::map<std::string, int> BoneNameToID;	// 뼈 이름과 인덱스 정수.
	std::vector<std::string> BoneIDToNames;	// BoneNameToID의 ID 순서대로 뼈 이름 저장.
	std::vector<int> BoneParents;				// 부모 뼈의 인덱스.
	std::vector<Matrix> OffsetMatrices;
	std::vector<Matrix> BoneTransforms;
	std::vector<AnimationClip> Clips;
	Matrix DefaultTransform;
	Matrix RootTransform;
	Matrix AccumulatedRootTransform;
	Vector3 PrevPos;
};