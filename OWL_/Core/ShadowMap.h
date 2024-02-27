#pragma once

#include "Texture2D.h"

namespace Core
{
	class ShadowMap
	{
	public:
		ShadowMap(UINT width = 1280, UINT height = 1280) : m_ShadowWidth(width), m_ShadowHeight(height), m_LightType(LIGHT_OFF) { }
		~ShadowMap() { Destroy(); }

		void Initialize(ID3D11Device* pDevice, UINT lightType);

		void Update(ID3D11DeviceContext* pContext, const LightProperty& PROPERTY, Camera& lightCam, Camera& mainCamera);

		void Render(ID3D11DeviceContext* pContext, std::vector<Geometry::Model*>& pBasicList, Geometry::Model* pMirror);

		void Destroy();

		inline UINT GetShadowWidth() { return m_ShadowWidth; }
		inline UINT GetShadowHeight() { return m_ShadowHeight; }

		inline Graphics::Texture2D& GetSpotLightShadowBuffer() { return m_SpotLightShadowBuffer; }
		inline Graphics::Texture2D* GetAdressOfSpotLightShadowBuffer() { return &m_SpotLightShadowBuffer; }

		inline Graphics::Texture2D& GetPointLightShadowBuffer() { return m_PointLightShadowBuffer; }
		inline Graphics::Texture2D* GetAddressOfPointLightShadowBuffer() { return &m_PointLightShadowBuffer; }

		inline Graphics::Texture2D& GetDirectionalLightShadowBuffer() { return m_DirectionalLightShadowBuffer; }
		inline Graphics::Texture2D* GetAddressOfDirectionalLightShadowBuffer() { return &m_DirectionalLightShadowBuffer; }

		inline ConstantsBuffer<GlobalConstants>* GetAddressOfShadowConstantBuffers() { return m_pShadowConstantsBuffers; }

		inline void SetShadowWidth(const UINT WIDTH) { m_ShadowWidth = WIDTH; }
		inline void SetShadowHeight(const UINT HEIGHT) { m_ShadowHeight = HEIGHT; }

	protected:
		void setPipelineState(ID3D11DeviceContext* pContext, const Graphics::GraphicsPSO& PSO);
		void setShadowViewport(ID3D11DeviceContext* pContext);

		void calculateCascadeLightViewProjection(Vector3* pPosition, Matrix* pView, Matrix* pProjection, const Matrix& VIEW, const Matrix& PROJECTION, const Vector3& DIR, int cascadeIndex);

	private:
		UINT m_ShadowWidth;
		UINT m_ShadowHeight;
		UINT m_LightType;
		
		Graphics::Texture2D m_SpotLightShadowBuffer;
		Graphics::Texture2D m_PointLightShadowBuffer;
		Graphics::Texture2D m_DirectionalLightShadowBuffer;
		ConstantsBuffer<GlobalConstants> m_pShadowConstantsBuffers[6]; // spot, point, direc => 0, 6, 3개씩 사용.
		ConstantsBuffer<ShadowConstants> m_ShadowConstantsBufferForGS; // 2개 이상의 view 행렬을 사용하는 광원을 위한  geometry용 상수버퍼.
	};
}
