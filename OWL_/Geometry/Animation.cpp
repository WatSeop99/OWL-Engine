#include "../Common.h"
#include "Animation.h"

namespace Geometry
{
	void AnimationData::Update(int clipID, int frame)
	{
		struct AnimationClip& clip = pClips[clipID];

		for (size_t boneID = 0, totalTransformSize = pBoneTransforms.size(); boneID < totalTransformSize; ++boneID)
		{
			std::vector<AnimationClip::Key>& keys = clip.pKeys[boneID];
			const size_t KEY_SIZE = keys.size();

			// 주의: 모든 채널(뼈)이 frame 개수가 동일하진 않음.
			const int PARENT_IDX = pBoneParents[boneID];
			const Matrix PARENT_MATRIX = (PARENT_IDX >= 0 ? pBoneTransforms[PARENT_IDX] : AccumulatedRootTransform);

			// keys.size()가 0일 경우에는 Identity 변환.
			AnimationClip::Key key = (KEY_SIZE > 0 ? keys[frame % KEY_SIZE] : AnimationClip::Key());

			// Root일 경우.
			if (PARENT_IDX < 0)
			{
				if (frame != 0)
				{
					AccumulatedRootTransform = (Matrix::CreateTranslation(key.Position - PrevPos) *
												AccumulatedRootTransform); // root 뼈의 변환을 누적시킴.
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

			pBoneTransforms[boneID] = key.GetTransform() * PARENT_MATRIX;
		}
	}
}
