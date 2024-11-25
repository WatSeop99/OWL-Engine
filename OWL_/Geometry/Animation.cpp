#include "../Common.h"
#include "Animation.h"

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Quaternion;
using DirectX::SimpleMath::Vector3;

Matrix AnimationClip::Key::GetTransform()
{
	return (Matrix::CreateScale(Scale) * Matrix::CreateFromQuaternion(Rotation) * Matrix::CreateTranslation(Position));
}

void AnimationData::Update(const int CLIP_ID, const int FRAME, const float DELTA_TIME)
{
	TimeSinceLoaded += DELTA_TIME;

	AnimationClip& clip = Clips[CLIP_ID];
	float timeInTicks = (float)(TimeSinceLoaded * clip.TicksPerSec);
	float animationTimeTicks = fmod(timeInTicks, (float)clip.Duration);

	for (SIZE_T boneID = 0, totalTransformSize = BoneTransforms.size(); boneID < totalTransformSize; ++boneID)
	{
		std::vector<AnimationClip::Key>& keys = clip.Keys[boneID];
		const SIZE_T KEY_SIZE = keys.size();

		// 주의: 모든 채널(뼈)이 FRAME 개수가 동일하진 않음.
		const int PARENT_IDX = BoneParents[boneID];
		const Matrix PARENT_MATRIX = (PARENT_IDX >= 0 ? BoneTransforms[PARENT_IDX] : AccumulatedRootTransform);

		// keys.size()가 0일 경우에는 Identity 변환.
		AnimationClip::Key key = (KEY_SIZE > 0 ? keys[FRAME % KEY_SIZE] : AnimationClip::Key());

		// Root일 경우.
		if (PARENT_IDX < 0)
		{
			if (FRAME != 0)
			{
				AccumulatedRootTransform = (Matrix::CreateTranslation(key.Position - PrevPos) * AccumulatedRootTransform); // root 뼈의 변환을 누적시킴.
			}
			else
			{
				// 애니메이션 섞어서 사용 시, 묘하게 틀어지는 부분이 존재. 이를 해결하기 위함.
				// 여기서는 캐릭터의 높이만 보정함.
				// 정확한 보정을 위해서는 환경(지면 등)에 따라 보정이 필요.
				Vector3 temp = AccumulatedRootTransform.Translation();
				temp.y = key.Position.y; // 높이 방향만 첫 프레임으로 보정.

				AccumulatedRootTransform.Translation(temp);
			}

			PrevPos = key.Position;
			key.Position = Vector3(0.0f); // 대신에 이동 취소.
		}

		BoneTransforms[boneID] = key.GetTransform() * PARENT_MATRIX;
	}

	//// root bone id은 0(아닐 수 있음).
	//{
	//	const int ROOT_BONE_ID = 0;

	//	Vector3 interpolatedPos;
	//	Vector3 interpolatedScale;
	//	Quaternion interpolatedRot;
	//	InterpolateKeyData(&interpolatedPos, &interpolatedRot, &interpolatedScale, &clip, 0, animationTimeTicks);

	//	if (CLIP_ID == 0)
	//	{
	//		interpolatedPos.y = 0.0f;
	//		BoneTransforms[ROOT_BONE_ID] = Matrix::CreateScale(interpolatedScale) * Matrix::CreateFromQuaternion(interpolatedRot) * Matrix::CreateTranslation(interpolatedPos);
	//	}
	//	else
	//	{
	//		BoneTransforms[ROOT_BONE_ID] = Matrix::CreateScale(interpolatedScale) * Matrix::CreateFromQuaternion(interpolatedRot);
	//	}
	//}

	//// 나머지 bone transform 업데이트.
	//// bone id가 부모->자식 순으로 선형적으로 저장되어 있기에 가능함.
	//for (UINT64 boneID = 1, totalBone = BoneTransforms.size(); boneID < totalBone; ++boneID)
	//{
	//	const int PARENT_ID = BoneParents[boneID];

	//	Vector3 interpolatedPos;
	//	Vector3 interpolatedScale;
	//	Quaternion interpolatedRot;
	//	InterpolateKeyData(&interpolatedPos, &interpolatedRot, &interpolatedScale, &clip, (const int)boneID, animationTimeTicks);

	//	BoneTransforms[boneID] = Matrix::CreateScale(interpolatedScale) * Matrix::CreateFromQuaternion(interpolatedRot) * Matrix::CreateTranslation(interpolatedPos) * BoneTransforms[PARENT_ID];
	//}
}

void AnimationData::InterpolateKeyData(Vector3* pOutPosition, Quaternion* pOutRotation, Vector3* pOutScale, AnimationClip* pClip, const int BONE_ID, const float ANIMATION_TIME_TICK)
{
	_ASSERT(pOutScale);
	_ASSERT(pClip);

	std::vector<AnimationClip::Key>& keys = pClip->Keys[BONE_ID];
	const UINT64 KEY_SIZE = keys.size();

	if (KEY_SIZE == 1)
	{
		AnimationClip::Key& key = keys[0];
		*pOutPosition = key.Position;
		*pOutRotation = key.Rotation;
		*pOutScale = key.Scale;

		return;
	}

	// Find key index at this time.
	UINT positionIndex = findIndex(pClip, BONE_ID, ANIMATION_TIME_TICK);
	UINT nextPositionIndex = positionIndex + 1;
	_ASSERT(nextPositionIndex < KEY_SIZE);

	AnimationClip::Key& curKey = keys[positionIndex];
	AnimationClip::Key& nextKey = keys[nextPositionIndex];

	// Calculate interpolated time.
	float t1 = (float)curKey.Time;
	float t2 = (float)nextKey.Time;
	float deltaTime = t2 - t1;
	float factor = (ANIMATION_TIME_TICK - t1) / deltaTime;

	// Calculate position data.
	const Vector3& START_POS = curKey.Position;
	const Vector3& END_POS = nextKey.Position;
	Vector3 deltaPos = END_POS - START_POS;
	*pOutPosition = START_POS + factor * deltaPos;

	// Calculate rotation data.
	const Quaternion& START_ROT = curKey.Rotation;
	const Quaternion& END_ROT = nextKey.Rotation;
	Quaternion interporlated = DirectX::XMQuaternionSlerp(START_ROT, END_ROT, factor);
	interporlated.Normalize();
	*pOutRotation = interporlated;

	// Calculate scale data.
	const Vector3& START_SCALE = curKey.Scale;
	const Vector3& END_SCALE = nextKey.Scale;
	Vector3 deltaScale = END_SCALE - START_SCALE;
	*pOutScale = START_SCALE + factor * deltaScale;
}

UINT AnimationData::findIndex(AnimationClip* pClip, const int BONE_ID, const float ANIMATION_TIME_TICK)
{
	_ASSERT(pClip);
	_ASSERT(BONE_ID >= 0);

	UINT ret = 0;

	std::vector<AnimationClip::Key>& keys = pClip->Keys[BONE_ID];
	for (UINT64 i = 0, end = keys.size() - 1; i < end; ++i)
	{
		float t = (float)keys[i + 1].Time;
		if (ANIMATION_TIME_TICK < t)
		{
			ret = (UINT)i;
			break;
		}
	}

	return ret;
}
