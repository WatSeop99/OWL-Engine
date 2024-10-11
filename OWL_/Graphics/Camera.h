#pragma once

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;

struct FrustumDirection
{
	Vector3 FrustumA;
	Vector3 FrustumB;
	Vector3 FrustumC;
	Vector3 FrustumD;
};

struct Keyboard;

class Camera
{
public:
	Camera() { UpdateViewDir(); }
	~Camera() = default;

	void Reset(const Vector3& POS, const float YAW, const float PITCH);

	void UpdateViewDir();
	void UpdateKeyboard(const float DELTA_TIME, Keyboard* const pKeyboard);
	void UpdateMouse(const float mouseNDCX, const float mouseNDCY);

	void MoveForward(const float DELTA_TIME);
	void MoveRight(const float DELTA_TIME);
	void MoveUp(const float DELTA_TIME);

	void PrintView();

	Matrix GetView();
	Matrix GetProjection();
	inline Vector3 GetEyePos() { return m_Position; }
	inline Vector3 GetViewDir() { return m_ViewDirection; }
	inline Vector3 GetUpDir() { return m_UpDirection; }
	inline Vector3 GetRightDir() { return m_RightDirection; }
	inline float GetProjectionFovAngleY() { return m_ProjectionFovAngleY; }
	inline float GetAspectRatio() { return m_Aspect; }
	inline float GetNearZ() { return m_NearZ; }
	inline float GetFarZ() { return m_FarZ; }
	FrustumDirection GetFrustumDirection();

	inline void SetAspectRatio(const float ASPECT_RATIO) { m_Aspect = ASPECT_RATIO; }
	inline void SetEyePos(const Vector3& POS) { m_Position = POS; }
	inline void SetViewDir(const Vector3& VIEW_DIR) { m_ViewDirection = VIEW_DIR; }
	inline void SetUpDir(const Vector3& UP_DIR) { m_UpDirection = UP_DIR; }
	inline void SetProjectionFovAngleY(const float ANGLE) { m_ProjectionFovAngleY = ANGLE; }
	inline void SetNearZ(const float NEAR_Z) { m_NearZ = NEAR_Z; }
	inline void SetFarZ(const float FAR_Z) { m_FarZ = FAR_Z; }

public:
	bool bUseFirstPersonView = false;

private:
	bool m_bUsePerspectiveProjection = true;

	Vector3 m_Position = Vector3(0.312183f, 0.957463f, -1.88458f);
	Vector3 m_ViewDirection = Vector3::UnitZ;
	Vector3 m_UpDirection = Vector3::UnitY; // +Y 방향으로 고정.
	Vector3 m_RightDirection = Vector3::UnitX;

	// roll, pitch, yaw
	// https://en.wikipedia.org/wiki/Aircraft_principal_axes
	float m_Yaw = -0.0589047f;
	float m_Pitch = 0.213803f;

	// 움직이는 속도.
	float m_Speed = 3.0f;

	// 프로젝션 옵션도 카메라 클래스로 이동.
	float m_ProjectionFovAngleY = 45.0f; // Luna 교재 기본 설정.
	float m_NearZ = 0.01f;
	float m_FarZ = 100.0f;
	float m_Aspect = 16.0f / 9.0f;
};

