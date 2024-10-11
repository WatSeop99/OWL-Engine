#include "../Common.h"
#include "../Renderer/BaseRenderer.h"
#include "ConstantDataType.h"
#include "../Geometry/Model.h"
#include "Light.h"

Light::Light(UINT width, UINT height) : m_ShadowMap(width, height)
{
	m_LightViewCamera.bUseFirstPersonView = true;
	m_LightViewCamera.SetAspectRatio((float)width / (float)height);
	m_LightViewCamera.SetEyePos(Property.Position);
	m_LightViewCamera.SetViewDir(Property.Direction);
	m_LightViewCamera.SetProjectionFovAngleY(120.0f);
	m_LightViewCamera.SetNearZ(0.1f);
	m_LightViewCamera.SetFarZ(50.0f);
}

void Light::Initialize(BaseRenderer* pRenderer)
{
	_ASSERT(pRenderer);

	switch (Property.LightType & m_TOTAL_LIGHT_TYPE)
	{
		case LIGHT_DIRECTIONAL:
			//m_LightViewCamera.SetFarZ(500.0f);
			m_ShadowMap.SetShadowWidth(2560);
			m_ShadowMap.SetShadowHeight(2560);
			break;

		case LIGHT_POINT:
			m_LightViewCamera.SetProjectionFovAngleY(90.0f);
			break;

		case LIGHT_SPOT:
		default:
			break;
	}

	m_ShadowMap.Initialize(pRenderer, Property.LightType);
}

void Light::Update(float deltaTime, Camera* pMainCamera)
{
	static Vector3 s_LightDev = Vector3::UnitX;
	if (bRotated)
	{
		s_LightDev = Vector3::Transform(s_LightDev, Matrix::CreateRotationY(deltaTime * DirectX::XM_PI * 0.5f));

		Vector3 focusPosition = Vector3(0.0f, -0.5f, 1.7f);
		Property.Position = Vector3(0.0f, 1.1f, 2.0f) + s_LightDev;
		Property.Direction = focusPosition - Property.Position;
	}

	Property.Direction.Normalize();
	m_LightViewCamera.SetEyePos(Property.Position);
	m_LightViewCamera.SetViewDir(Property.Direction);


	if (!(Property.LightType & LIGHT_SHADOW))
	{
		return;
	}

	Vector3 up = m_LightViewCamera.GetUpDir();
	if (abs(up.Dot(Property.Direction) + 1.0f) < 1e-5)
	{
		up = Vector3::UnitX;
		m_LightViewCamera.SetUpDir(up);
	}

	// 그림자 맵 생성시 필요.
	Matrix lightView = DirectX::XMMatrixLookAtLH(Property.Position, Property.Position + Property.Direction, up); // 카메라를 이용하면 pitch, yaw를 고려하게됨. 이를 방지하기 위함.
	Matrix lightProjection = m_LightViewCamera.GetProjection();
	m_ShadowMap.Update(Property, &m_LightViewCamera, pMainCamera);

	switch (Property.LightType & m_TOTAL_LIGHT_TYPE)
	{
		case LIGHT_DIRECTIONAL:
			Property.ViewProjections[0] = (lightView * lightProjection).Transpose();
			Property.Projections[0] = lightProjection.Transpose();
			Property.InverseProjections[0] = lightProjection.Invert().Transpose();
			break;

		case LIGHT_POINT:
		{
			ConstantBuffer* const pShadowCubeConstants = m_ShadowMap.GetShadowConstantBuffers();
			for (int i = 0; i < 6; ++i)
			{
				GlobalConstants* const pConstantData = (GlobalConstants*)pShadowCubeConstants[i].pSystemMem;
				Property.ViewProjections[i] = (pConstantData->View.Transpose() * lightProjection).Transpose();
			}
			Property.Projections[0] = lightProjection.Transpose();
			Property.InverseProjections[0] = lightProjection.Invert().Transpose();
		}
		break;

		case LIGHT_SPOT:
			Property.ViewProjections[0] = (lightView * lightProjection).Transpose();
			Property.Projections[0] = lightProjection.Transpose();
			Property.InverseProjections[0] = lightProjection.Invert().Transpose();
			break;

		default:
			break;
	}

	// LIGHT_FRUSTUM_WIDTH 확인
	/*DirectX::SimpleMath::Vector4 eye(0.0f, 0.0f, 0.0f, 1.0f);
	DirectX::SimpleMath::Vector4 xLeft(-1.0f, -1.0f, 0.0f, 1.0f);
	DirectX::SimpleMath::Vector4 xRight(1.0f, 1.0f, 0.0f, 1.0f);
	eye = DirectX::SimpleMath::Vector4::Transform(eye, lightProjection);
	xLeft = DirectX::SimpleMath::Vector4::Transform(xLeft, lightProjection.Invert());
	xRight = DirectX::SimpleMath::Vector4::Transform(xRight, lightProjection.Invert());
	xLeft /= xLeft.w;
	xRight /= xRight.w;
	std::cout << "LIGHT_FRUSTUM_WIDTH = " << xRight.x - xLeft.x << std::endl;*/
}

void Light::RenderShadowMap(std::vector<Model*>& pBasicList, Model* pMirror)
{
	if (Property.LightType & LIGHT_SHADOW)
	{
		m_ShadowMap.Render(pBasicList, pMirror);
	}
}

void Light::Cleanup()
{
	m_ShadowMap.Cleanup();
}
