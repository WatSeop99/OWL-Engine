#pragma once

#include "../Renderer/ConstantBuffer.h"
#include "../Renderer/Texture2D.h"

class ShadowMap
{
public:
	ShadowMap(UINT width = 1280, UINT height = 1280) : m_ShadowWidth(width), m_ShadowHeight(height), m_LightType(LIGHT_OFF) {}
	~ShadowMap() { Cleanup(); }

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, UINT lightType);

	void Update(ID3D11DeviceContext* pContext, const LightProperty& PROPERTY, Camera& lightCam, Camera& mainCamera);

	void Render(ID3D11DeviceContext* pContext, std::vector<Model*>& pBasicList, Model* pMirror);

	void Cleanup();

	inline UINT GetShadowWidth() { return m_ShadowWidth; }
	inline UINT GetShadowHeight() { return m_ShadowHeight; }

	inline Texture2D* GetSpotLightShadowBufferPtr() { return &m_SpotLightShadowBuffer; }
	inline Texture2D* GetPointLightShadowBufferPtr() { return &m_PointLightShadowBuffer; }
	inline Texture2D* GetDirectionalLightShadowBufferPtr() { return &m_DirectionalLightShadowBuffer; }

	//inline ConstantsBuffer<GlobalConstants>* GetShadowConstantBuffersPtr() { return m_pShadowConstantsBuffers; }
	inline ConstantBuffer* GetShadowConstantBuffersPtr() { return m_pShadowConstantsBuffers; }

	inline void SetShadowWidth(const UINT WIDTH) { m_ShadowWidth = WIDTH; }
	inline void SetShadowHeight(const UINT HEIGHT) { m_ShadowHeight = HEIGHT; }

protected:
	void setPipelineState(ID3D11DeviceContext* pContext, const GraphicsPSO& PSO);
	void setShadowViewport(ID3D11DeviceContext* pContext);

	void calculateCascadeLightViewProjection(Vector3* pPosition, Matrix* pView, Matrix* pProjection, const Matrix& VIEW, const Matrix& PROJECTION, const Vector3& DIR, int cascadeIndex);

private:
	UINT m_ShadowWidth = 0;
	UINT m_ShadowHeight = 0;
	UINT m_LightType = LIGHT_OFF;

	Texture2D m_SpotLightShadowBuffer;
	Texture2D m_PointLightShadowBuffer;
	Texture2D m_DirectionalLightShadowBuffer;
	//ConstantsBuffer<GlobalConstants> m_pShadowConstantsBuffers[6]; // spot, point, direc => 0, 6, 4개씩 사용.
	//ConstantsBuffer<ShadowConstants> m_ShadowConstantsBufferForGS; // 2개 이상의 view 행렬을 사용하는 광원을 위한  geometry용 상수버퍼.
	ConstantBuffer m_pShadowConstantsBuffers[6]; // spot, point, direc => 0, 6, 4개씩 사용.
	ConstantBuffer m_ShadowConstantsBufferForGS; // 2개 이상의 view 행렬을 사용하는 광원을 위한  geometry용 상수버퍼.
};