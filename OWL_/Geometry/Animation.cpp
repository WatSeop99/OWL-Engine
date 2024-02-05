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

			// ����: ��� ä��(��)�� frame ������ �������� ����.
			const int PARENT_IDX = pBoneParents[boneID];
			const Matrix PARENT_MATRIX = (PARENT_IDX >= 0 ? pBoneTransforms[PARENT_IDX] : AccumulatedRootTransform);

			// keys.size()�� 0�� ��쿡�� Identity ��ȯ.
			AnimationClip::Key key = (KEY_SIZE > 0 ? keys[frame % KEY_SIZE] : AnimationClip::Key());

			// Root�� ���.
			if (PARENT_IDX < 0)
			{
				if (frame != 0)
				{
					AccumulatedRootTransform = (Matrix::CreateTranslation(key.Position - PrevPos) *
												AccumulatedRootTransform); // root ���� ��ȯ�� ������Ŵ.
				}
				else
				{
					// �ִϸ��̼� ��� ��� ��, ���ϰ� Ʋ������ �κ��� ����. �̸� �ذ��ϱ� ����.
					// ���⼭�� ĳ������ ���̸� ������.
					// ��Ȯ�� ������ ���ؼ��� ȯ��(���� ��)�� ���� ������ �ʿ�.
					Vector3 temp = AccumulatedRootTransform.Translation();
					temp.y = key.Position.y; // ���� ���⸸ ù ���������� ����.

					AccumulatedRootTransform.Translation(temp);
				}

				PrevPos = key.Position;
				key.Position = Vector3(0.0f); // ��ſ� �̵� ���.
			}

			pBoneTransforms[boneID] = key.GetTransform() * PARENT_MATRIX;
		}
	}
}
