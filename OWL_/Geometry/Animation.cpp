#include "../Common.h"
#include "Animation.h"

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Quaternion;
using DirectX::SimpleMath::Vector3;

Matrix AnimationClip::Key::GetTransform()
{
	return (Matrix::CreateScale(Scale) * Matrix::CreateFromQuaternion(Rotation) * Matrix::CreateTranslation(Position));
}

void AnimationData::Update(const int CLIP_ID, const int FRAME)
{
	AnimationClip& clip = Clips[CLIP_ID];

	for (size_t boneID = 0, totalTransformSize = BoneTransforms.size(); boneID < totalTransformSize; ++boneID)
	{
		std::vector<AnimationClip::Key>& keys = clip.Keys[boneID];
		const size_t KEY_SIZE = keys.size();

		// ����: ��� ä��(��)�� FRAME ������ �������� ����.
		const int PARENT_IDX = BoneParents[boneID];
		const Matrix PARENT_MATRIX = (PARENT_IDX >= 0 ? BoneTransforms[PARENT_IDX] : AccumulatedRootTransform);

		// keys.size()�� 0�� ��쿡�� Identity ��ȯ.
		AnimationClip::Key key = (KEY_SIZE > 0 ? keys[FRAME % KEY_SIZE] : AnimationClip::Key());

		// Root�� ���.
		if (PARENT_IDX < 0)
		{
			if (FRAME != 0)
			{
				AccumulatedRootTransform = (Matrix::CreateTranslation(key.Position - PrevPos) * AccumulatedRootTransform); // root ���� ��ȯ�� ������Ŵ.
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

		BoneTransforms[boneID] = key.GetTransform() * PARENT_MATRIX;
	}
}