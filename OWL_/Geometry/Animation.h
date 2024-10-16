#pragma once

#include <directxtk/SimpleMath.h>

struct AnimationClip
{
	class Key
	{
	public:
		Key() = default;
		~Key() = default;

		DirectX::SimpleMath::Matrix GetTransform();

	public:
		DirectX::SimpleMath::Vector3 Position;
		DirectX::SimpleMath::Vector3 Scale = DirectX::SimpleMath::Vector3(1.0f);
		DirectX::SimpleMath::Quaternion Rotation;
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

	inline DirectX::SimpleMath::Matrix Get(const int CLIP_ID, const int BONE_ID, const int FRAME)
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
	std::vector<DirectX::SimpleMath::Matrix> OffsetMatrices;
	std::vector<DirectX::SimpleMath::Matrix> BoneTransforms;
	std::vector<AnimationClip> Clips;
	DirectX::SimpleMath::Matrix DefaultTransform;
	DirectX::SimpleMath::Matrix RootTransform;
	DirectX::SimpleMath::Matrix AccumulatedRootTransform;
	DirectX::SimpleMath::Vector3 PrevPos;
};