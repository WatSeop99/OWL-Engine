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
			// DefaultTransform�� ���� �о���϶� GeometryGenerator::Normalize()���� ���. 
			// DefaultTransform.Invert() * OffsetMatrices[boneID]�� �̸� ����ؼ� ��ġ�� 
			// DefaultTransform * RootTransform�� �̸� ����س��� �� ����.
			return (DefaultTransform.Invert() * pOffsetMatrices[boneID] *
					pBoneTransforms[boneID] * DefaultTransform);
		}

		void Update(int clipID, int frame);

	public:
		std::map<std::string, int32_t> BoneNameToID; // �� �̸��� �ε��� ����.
		std::vector<std::string> pBoneIDToNames;	 // BoneNameToID�� ID ������� �� �̸� ����.
		std::vector<int32_t> pBoneParents;			 // �θ� ���� �ε���.
		std::vector<Matrix> pOffsetMatrices;
		std::vector<Matrix> pBoneTransforms;
		std::vector<AnimationClip> pClips;
		Matrix DefaultTransform;
		Matrix RootTransform;
		Matrix AccumulatedRootTransform;
		Vector3 PrevPos;
	};
}