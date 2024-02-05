#pragma once

#include <directxtk/SimpleMath.h>

namespace Geometry
{
	using DirectX::SimpleMath::Matrix;
	using DirectX::SimpleMath::Quaternion;
	using DirectX::SimpleMath::Vector3;
	using std::map;
	using std::string;
	using std::vector;

	struct AnimationClip
	{
		class Key
		{
		public:
			inline Key() : Position(0.0f), Scale(1.0f), Rotation() { }

			inline Matrix GetTransform()
			{
				return (Matrix::CreateScale(Scale) *
						Matrix::CreateFromQuaternion(Rotation) *
						Matrix::CreateTranslation(Position));
			}

		public:
			Vector3 Position;
			Vector3 Scale;
			Quaternion Rotation;
		};

		std::string Name;					 // Name of this animation clip.
		std::vector<std::vector<Key>> pKeys; // m_key[boneIdx][frameIdx].
		int NumChannels;					 // Number of bones.
		int NumKeys;						 // Number of frames of this animation clip.
		double Duration;					 // Duration of animation in ticks.
		double TicksPerSec;					 // Frames per second.
	};

	class AnimationData
	{
	public:
		AnimationData() : 
			DefaultTransform(), 
			RootTransform(), 
			AccumulatedRootTransform(), 
			PrevPos(0.0f)
		{ }

		inline Matrix Get(int clipID, int boneID, int frame)
		{
			// DefaultTransform은 모델을 읽어들일때 GeometryGenerator::Normalize()에서 계산. 
			// DefaultTransform.Invert() * OffsetMatrices[boneID]를 미리 계산해서 합치고 
			// DefaultTransform * RootTransform을 미리 계산해놓을 수 있음.
			return (DefaultTransform.Invert() * pOffsetMatrices[boneID] *
					pBoneTransforms[boneID] * DefaultTransform);
		}

		void Update(int clipID, int frame);

	public:
		std::map<std::string, int32_t> BoneNameToID; // 뼈 이름과 인덱스 정수.
		std::vector<std::string> pBoneIDToNames;	 // BoneNameToID의 ID 순서대로 뼈 이름 저장.
		std::vector<int32_t> pBoneParents;			 // 부모 뼈의 인덱스.
		std::vector<Matrix> pOffsetMatrices;
		std::vector<Matrix> pBoneTransforms;
		std::vector<AnimationClip> pClips;
		Matrix DefaultTransform;
		Matrix RootTransform;
		Matrix AccumulatedRootTransform;
		Vector3 PrevPos;
	};
}