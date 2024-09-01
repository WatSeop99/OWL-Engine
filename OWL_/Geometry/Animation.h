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
		// DefaultTransform�� ���� �о���϶� GeometryGenerator::Normalize()���� ���. 
		// DefaultTransform.Invert() * OffsetMatrices[BONE_ID]�� �̸� ����ؼ� ��ġ�� 
		// DefaultTransform * RootTransform�� �̸� ����س��� �� ����.
		return (DefaultTransform.Invert() * OffsetMatrices[BONE_ID] * BoneTransforms[BONE_ID] * DefaultTransform);
	}

public:
	std::map<std::string, int> BoneNameToID;	// �� �̸��� �ε��� ����.
	std::vector<std::string> BoneIDToNames;	// BoneNameToID�� ID ������� �� �̸� ����.
	std::vector<int> BoneParents;				// �θ� ���� �ε���.
	std::vector<Matrix> OffsetMatrices;
	std::vector<Matrix> BoneTransforms;
	std::vector<AnimationClip> Clips;
	Matrix DefaultTransform;
	Matrix RootTransform;
	Matrix AccumulatedRootTransform;
	Vector3 PrevPos;
};