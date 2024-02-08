#pragma once

#include "Texture2D.h"

namespace Core
{
	class ShadowMap
	{
	public:
		ShadowMap(UINT width = 1280, UINT height = 1280) : m_ShadowWidth(width), m_ShadowHeight(height) { }
		~ShadowMap() { Destroy(); }

		void Initialize(ID3D11Device* pDevice);

		void Update(ID3D11DeviceContext* pContext, const Vector3& POS, const Matrix& VIEW, const Matrix& PROJECTION);

		void Render(ID3D11DeviceContext* pContext, std::vector<Geometry::Model*>& pBasicList, Geometry::Model* pMirror);

		void Destroy();

		inline UINT GetShadowWidth() { return m_ShadowWidth; }
		inline UINT GetShadowHeight() { return m_ShadowHeight; }
		inline Texture2D& GetShadowBuffer() { return m_ShadowBuffer; }
		inline Texture2D* GetAddressOfShadowBuffer() { return &m_ShadowBuffer; }
		inline ConstantsBuffer<GlobalConstants>& GetShadowConstantBuffer() { return m_ShadowConstantsBuffer; }
		inline ConstantsBuffer<GlobalConstants>* GetAddressOfShadowConstantBuffer() { return &m_ShadowConstantsBuffer; }

		inline void SetShadowWidth(const UINT WIDTH) { m_ShadowWidth = WIDTH; }
		inline void SetShadowHeight(const UINT HEIGHT) { m_ShadowHeight = HEIGHT; }

	protected:
		void setPipelineState(ID3D11DeviceContext* pContext, const Graphics::GraphicsPSO& PSO);
		void setShadowViewport(ID3D11DeviceContext* pContext);

	private:
		UINT m_ShadowWidth;
		UINT m_ShadowHeight;
		Texture2D m_ShadowBuffer;
		ConstantsBuffer<GlobalConstants> m_ShadowConstantsBuffer;
	};
}
