#include "../Common.h"
#include "Light.h"

namespace Core
{
	Light::Light(UINT width, UINT height) :
		bRotated(false),
		bVisible(false),
		m_ShadowMap(width, height)
	{
		m_LightViewCamera.bUseFirstPersonView = true;
		m_LightViewCamera.SetAspectRatio((float)width / (float)height);
		m_LightViewCamera.SetEyePos(Property.Position);
		m_LightViewCamera.SetViewDir(Property.Direction);
		m_LightViewCamera.SetProjectionFovAngleY(120.0f);
		m_LightViewCamera.SetNearZ(0.1f);
		m_LightViewCamera.SetFarZ(50.0f);
	}

	void Light::Initialize(ID3D11Device* pDevice)
	{
		Destroy();

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

		m_ShadowMap.Initialize(pDevice, Property.LightType);
	}

	void Light::Update(ID3D11DeviceContext* pContext, float deltaTime, Camera& mainCamera)
	{
		static Vector3 s_LightDev = Vector3(1.0f, 0.0f, 0.0f);
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
				up = Vector3(1.0f, 0.0f, 0.0f);
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
				ConstantsBuffer<GlobalConstants>* const pShadowConstants = m_ShadowMap.GetAddressOfShadowConstantBuffers();
				for (int i = 0; i < 4; ++i)
				{
					ConstantsBuffer<GlobalConstants>* const pConstant = &pShadowConstants[i];
					Property.ViewProjections[i] = pConstant->CPU.ViewProjection;
					Property.Projections[i] = pConstant->CPU.Projection;
					Property.InverseProjections[i] = pConstant->CPU.InverseProjection;
				}
			}
				break;

			case LIGHT_POINT:
			{
				ConstantsBuffer<GlobalConstants>* const pShadowCubeConstants = m_ShadowMap.GetAddressOfShadowConstantBuffers();
				for (int i = 0; i < 6; ++i)
				{
					ConstantsBuffer<GlobalConstants>* const pConstant = &pShadowCubeConstants[i];
					Property.ViewProjections[i] = (pConstant->CPU.View.Transpose() * lightProjection).Transpose();
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

	void Light::RenderShadowMap(ID3D11DeviceContext* pContext, std::vector<Geometry::Model*>& pBasicList, Geometry::Model* pMirror)
	{
		if (Property.LightType & LIGHT_SHADOW)
		{
			m_ShadowMap.Render(pContext, pBasicList, pMirror);
		}
	}

	void Light::Render(ID3D11DeviceContext* pContext, std::vector<Geometry::Model*>& pBasicList, Geometry::Model* pMirror)
	{
		
	}

	void Light::Destroy()
	{
		m_ShadowMap.Destroy();
	}
}
