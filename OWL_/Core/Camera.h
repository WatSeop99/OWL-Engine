#pragma once

namespace Core
{
	using DirectX::SimpleMath::Matrix;
	using DirectX::SimpleMath::Vector3;

	class Camera
	{
	public:
		inline Camera() :
			bUseFirstPersonView(false),
			m_Position(0.312183f, 0.957463f, -1.88458f),
			m_ViewDirection(0.0f, 0.0f, 1.0f),
			m_UpDirection(0.0f, 1.0f, 0.0f),
			m_RightDirection(1.0f, 0.0f, 0.0f),
			m_Yaw(-0.0589047f), m_Pitch(0.213803f),
			m_Speed(3.0f),
			m_ProjectionFovAngleY(45.0f),
			m_NearZ(0.01f), m_FarZ(100.0f),
			m_Aspect(16.0f / 9.0f),
			m_bUsePerspectiveProjection(true)
		{ UpdateViewDir(); }

		inline void SetAspectRatio(float aspect) { m_Aspect = aspect; }

		inline Matrix GetView()
		{
			return (Matrix::CreateTranslation(-m_Position) *
					Matrix::CreateRotationY(-m_Yaw) *
					Matrix::CreateRotationX(-m_Pitch)); // m_Pitch가 양수이면 고개를 드는 방향.
		}
		inline Matrix GetProjection()
		{
			return (m_bUsePerspectiveProjection ?
					DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(m_ProjectionFovAngleY), m_Aspect, m_NearZ, m_FarZ) :
					DirectX::XMMatrixOrthographicOffCenterLH(-m_Aspect, m_Aspect, -1.0f, 1.0f, m_NearZ, m_FarZ));
		}
		inline Vector3 GetEyePos() { return m_Position; }

		inline void Reset(Vector3 pos, float yaw, float pitch)
		{
			m_Position = pos;
			m_Yaw = yaw;
			m_Pitch = pitch;
			UpdateViewDir();
		}

		void UpdateViewDir();
		void UpdateKeyboard(const float DELTA_TIME, bool const bKEY_PRESSED[256]);
		void UpdateMouse(float mouseNDCX, float mouseNDCY);

		void MoveForward(float deltaTime);
		void MoveRight(float deltaTime);
		void MoveUp(float deltaTime);

		void PrintView();

	public:
		bool bUseFirstPersonView;

	private:
		Vector3 m_Position;
		Vector3 m_ViewDirection;
		Vector3 m_UpDirection; // +Y 방향으로 고정.
		Vector3 m_RightDirection;

		// roll, pitch, yaw
		// https://en.wikipedia.org/wiki/Aircraft_principal_axes
		float m_Yaw;
		float m_Pitch;

		float m_Speed; // 움직이는 속도.

		// 프로젝션 옵션도 카메라 클래스로 이동.
		float m_ProjectionFovAngleY; // Luna 교재 기본 설정.
		float m_NearZ;
		float m_FarZ;
		float m_Aspect;
		bool m_bUsePerspectiveProjection;
	};

}
