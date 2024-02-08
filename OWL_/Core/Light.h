#pragma once

#include "Camera.h"
#include "ConstantBuffers.h"
#include "../Geometry/Model.h"
#include "ShadowMap.h"

namespace Core
{
	class Light
	{
	public:
		Light(UINT width = 1280, UINT height = 1280);
		~Light() { Destroy(); }

		void Initialize(ID3D11Device* pDevice);

		void Update(ID3D11DeviceContext* pContext, float deltaTime);

		void RenderShadowMap(ID3D11DeviceContext* pContext, std::vector<Geometry::Model*>& pBasicList, Geometry::Model* pMirror);
		void Render(ID3D11DeviceContext* pContext, std::vector<Geometry::Model*>& pBasicList, Geometry::Model* pMirror);

		void Destroy();

		inline Vector3 GetPosition() { return m_LightViewCamera.GetEyePos(); }
		inline Vector3 GetDirection() { return m_LightViewCamera.GetViewDir(); }
		inline ShadowMap& GetShadowMap() { return m_ShadowMap; }
		inline ShadowMap* GetAddressOfShadowMap() { return &m_ShadowMap; }

		inline void SetPosition(const Vector3& POS) { m_LightViewCamera.SetEyePos(POS); }
		inline void SetDirection(const Vector3& DIR) { m_LightViewCamera.SetViewDir(DIR); }
		inline void SetShadowSize(const UINT WIDTH, const UINT HEIGHT) { m_ShadowMap.SetShadowWidth(WIDTH); m_ShadowMap.SetShadowHeight(HEIGHT); }

	public:
		bool bRotated;
		LightProperty Property;

	private:
		Camera m_LightViewCamera;
		ShadowMap m_ShadowMap;
		// ConstantsBuffer<GlobalConstants> m_ShadowConstantsBuffer;
	};
}
