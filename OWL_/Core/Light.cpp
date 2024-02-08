#include "../Common.h"
#include "Light.h"

namespace Core
{
	Light::Light(UINT width, UINT height) : 
		bRotated(false),
		m_ShadowMap(width, height)
	{
		m_LightViewCamera.SetAspectRatio((float)width / (float)height);
		m_LightViewCamera.bUseFirstPersonView = true;
		m_LightViewCamera.SetEyePos(Property.Position);
		m_LightViewCamera.SetViewDir(Property.Direction);
		m_LightViewCamera.SetProjectionFovAngleY(120.0f);
		m_LightViewCamera.SetNearZ(0.1f);
		m_LightViewCamera.SetFarZ(10.0f);
	}

	void Light::Initialize(ID3D11Device* pDevice)
	{
		Destroy();
		m_ShadowMap.Initialize(pDevice);
	}

	void Light::Update(ID3D11DeviceContext* pContext, float deltaTime)
	{
		static Vector3 s_LightDev = Vector3(1.0f, 0.0f, 0.0f);
		if (bRotated)
		{
			s_LightDev = Vector3::Transform(s_LightDev, Matrix::CreateRotationY(deltaTime * DirectX::XM_PI * 0.5f));
		
			Vector3 focusPosition = Vector3(0.0f, -0.5f, 1.7f);
			Property.Position = Vector3(0.0f, 1.1f, 2.0f) + s_LightDev;
			Property.Direction = focusPosition - Property.Position;
			Property.Direction.Normalize();
		}

		m_LightViewCamera.SetEyePos(Property.Position);
		m_LightViewCamera.SetViewDir(Property.Direction);

		if (Property.LightType & LIGHT_SHADOW)
		{
			Vector3 up = m_LightViewCamera.GetUpDir();
			if (abs(up.Dot(Property.Direction) + 1.0f) < 1e-5)
			{
				up = Vector3(1.0f, 0.0f, 0.0f);
				m_LightViewCamera.SetUpDir(up);
			}

			// 그림자 맵 생성시 필요.
			// Matrix lightView = m_LightViewCamera.GetView();
			Matrix lightView = DirectX::XMMatrixLookAtLH(Property.Position, Property.Position + Property.Direction, up);
			Matrix lightProjection = m_LightViewCamera.GetProjection();

			/*m_ShadowConstantsBuffer.CPU.EyeWorld = pos;
			m_ShadowConstantsBuffer.CPU.View = lightView.Transpose();
			m_ShadowConstantsBuffer.CPU.Projection = lightProjection.Transpose();
			m_ShadowConstantsBuffer.CPU.InverseProjection = lightProjection.Invert().Transpose();
			m_ShadowConstantsBuffer.CPU.ViewProjection = (lightView * lightProjection).Transpose();

			m_ShadowConstantsBuffer.Upload(pContext);*/
			m_ShadowMap.Update(pContext, Property.Position, lightView, lightProjection);

			// 그림자를 실제로 렌더링할 때 필요.
			// 반사된 장면에서도 그림자를 그리고 싶다면 조명도 반사시켜서 넣어주면 됨.
			Property.ViewProjection = (lightView * lightProjection).Transpose();
			Property.InverseProjection = lightProjection.Invert().Transpose();
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
