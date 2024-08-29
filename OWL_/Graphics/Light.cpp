#include "../Common.h"
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

void Light::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	_ASSERT(pDevice);
	_ASSERT(pContext);

	switch (Property.LightType & (LIGHT_DIRECTIONAL | LIGHT_POINT | LIGHT_SPOT))
	{
		case LIGHT_DIRECTIONAL:
		{
			m_LightViewCamera.SetFarZ(500.0f);
			m_ShadowMap.SetShadowWidth(2560);
			m_ShadowMap.SetShadowHeight(2560);
		}
		break;

		case LIGHT_POINT:
		{
			m_LightViewCamera.SetProjectionFovAngleY(90.0f);
		}
		break;

		case LIGHT_SPOT:
		default:
			break;
	}

	m_ShadowMap.Initialize(pDevice, pContext, Property.LightType);
}

void Light::Update(ID3D11DeviceContext* pContext, float deltaTime, Camera& mainCamera)
{
	_ASSERT(pContext);

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

	if (Property.LightType & LIGHT_SHADOW)
	{
		Vector3 up = m_LightViewCamera.GetUpDir();
		if (fabs(up.Dot(Property.Direction) + 1.0f) < 1e-5)
		{
			up = Vector3::UnitX;
			m_LightViewCamera.SetUpDir(up);
		}

		// 그림자 맵 생성시 필요.
		Matrix lightView = DirectX::XMMatrixLookAtLH(Property.Position, Property.Position + Property.Direction, up); // 카메라를 이용하면 pitch, yaw를 고려하게됨. 이를 방지하기 위함.
		Matrix lightProjection = m_LightViewCamera.GetProjection();
		m_ShadowMap.Update(pContext, Property, m_LightViewCamera, mainCamera);

		switch (Property.LightType & (LIGHT_DIRECTIONAL | LIGHT_POINT | LIGHT_SPOT))
		{
			case LIGHT_DIRECTIONAL:
			{
				ConstantBuffer* const pShadowConstants = m_ShadowMap.GetShadowConstantBuffersPtr();
				for (int i = 0; i < 4; ++i)
				{
					GlobalConstants* const pConstantData = (GlobalConstants*)pShadowConstants[i].pSystemMem;
					Property.ViewProjections[i] = pConstantData->ViewProjection;
					Property.Projections[i] = pConstantData->Projection;
					Property.InverseProjections[i] = pConstantData->InverseProjection;
				}
			}
			break;

			case LIGHT_POINT:
			{
				ConstantBuffer* const pShadowCubeConstants = m_ShadowMap.GetShadowConstantBuffersPtr();
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
			{
				// 그림자를 실제로 렌더링할 때 필요.
				// 반사된 장면에서도 그림자를 그리고 싶다면 조명도 반사시켜서 넣어주면 됨.
				Property.ViewProjections[0] = (lightView * lightProjection).Transpose();
				Property.Projections[0] = lightProjection.Transpose();
				Property.InverseProjections[0] = lightProjection.Invert().Transpose();
			}
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
}

void Light::RenderShadowMap(ID3D11DeviceContext* pContext, std::vector<Model*>& pBasicList, Model* pMirror)
{
	if (Property.LightType & LIGHT_SHADOW)
	{
		m_ShadowMap.Render(pContext, pBasicList, pMirror);
	}
}

void Light::Cleanup()
{
	m_ShadowMap.Cleanup();
}
