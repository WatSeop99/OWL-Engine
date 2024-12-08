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
		double Time = 0.0f;
	};

	std::string Name;					 // Name of this animation clip.
	std::vector<std::vector<Key>> Keys;	 // Keys[boneIDX][frameIDX].
	int NumChannels;					 // Number of bones.
	double Duration;					 // Duration of animation in ticks.
	double TicksPerSec;					 // Frames per second.
};

class AnimationData
{
public:
	AnimationData() = default;
	~AnimationData() = default;

	void Update(const int CLIP_ID, const int FRAME, const float DELTA_TIME);
	void UpdateVelocity(const int CLIP_ID, const int FRAME);

	void InterpolateKeyData(Vector3* pOutPosition, Quaternion* pOutRotation, Vector3* pOutScale, AnimationClip* pClip, const int BONE_ID, const float ANIMATION_TIME_TICK);

	DirectX::SimpleMath::Matrix Get(const int CLIP_ID, const int BONE_ID, const int FRAME);

protected:
	UINT findIndex(AnimationClip* pClip, const int BONE_ID, const float ANIMATION_TIME_TICK);

public:
	std::map<std::string, int> BoneNameToID;	// 뼈 이름과 인덱스 정수.
	std::vector<std::string> BoneIDToNames;	// BoneNameToID의 ID 순서대로 뼈 이름 저장.
	std::vector<int> BoneParents;				// 부모 뼈의 인덱스.
	std::vector<Matrix> OffsetMatrices;					// 뼈와 skin 사이의 변환. 뼈 좌표계에서 mesh의 위치.	
	std::vector<Matrix> InverseOffsetMatrices;
	std::vector<Matrix> NodeTransforms;
	std::vector<Matrix> BoneTransforms;					// 해당 시점 key data의 움직임에 따른 뼈의 변환 행렬.
	std::vector<AnimationClip> Clips;					// 애니메이션 동작.

	DirectX::SimpleMath::Matrix DefaultTransform;
	DirectX::SimpleMath::Matrix InverseDefaultTransform;
	DirectX::SimpleMath::Matrix RootTransform;
	DirectX::SimpleMath::Matrix AccumulatedRootTransform;
	// DirectX::SimpleMath::Vector3 PrevPos;
	DirectX::SimpleMath::Vector3 PrevKeyPos;
	DirectX::SimpleMath::Vector3 Position;
	DirectX::SimpleMath::Vector3 Direction;
	DirectX::SimpleMath::Quaternion Rotation;

	double TimeSinceLoaded = 0.25f;
	float NormalizingScale = 1.0f;
	float Velocity = 0.0f;
};