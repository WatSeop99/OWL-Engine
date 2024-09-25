#pragma once

#include "../Renderer/ConstantBuffer.h"
#include "../Graphics/ConstantDataType.h"
#include "../Renderer/Texture.h"

struct LightProperty;
class BaseRenderer;
class Camera;
class GraphicsPSO;
class Model;

class ShadowMap
{
public:
	ShadowMap(const UINT WIDTH = 1280, const UINT HEIGHT = 1280) : m_ShadowWidth(WIDTH), m_ShadowHeight(HEIGHT) {}
	~ShadowMap() { Cleanup(); }

	void Initialize(BaseRenderer* pRenderer, const UINT LIGHT_TYPE);

	void Update(const LightProperty& PROPERTY, Camera& lightCam, Camera& mainCamera);

	void Render(std::vector<Model*>& pBasicList, Model* pMirror);

	void Cleanup();

	inline UINT GetShadowWidth() { return m_ShadowWidth; }
	inline UINT GetShadowHeight() { return m_ShadowHeight; }

	inline Texture* GetSpotLightShadowBuffer() { return &m_SpotLightShadowBuffer; }
	inline Texture* GetPointLightShadowBuffer() { return &m_PointLightShadowBuffer; }
	inline Texture* GetDirectionalLightShadowBuffer() { return &m_DirectionalLightShadowBuffer; }

	inline ConstantBuffer* GetShadowConstantBuffers() { return m_pShadowConstantsBuffers; }

	inline void SetShadowWidth(const UINT WIDTH) { m_ShadowWidth = WIDTH; }
	inline void SetShadowHeight(const UINT HEIGHT) { m_ShadowHeight = HEIGHT; }

protected:
	void setShadowViewport();

	void calculateCascadeLightViewProjection(Vector3* pPosition, Matrix* pView, Matrix* pProjection, const Matrix& VIEW, const Matrix& PROJECTION, const Vector3& DIR, int cascadeIndex);

private:
	const UINT m_TOTAL_LIGHT_TYPE = (LIGHT_DIRECTIONAL | LIGHT_POINT | LIGHT_SPOT | LIGHT_SUN);

	UINT m_ShadowWidth = 0;
	UINT m_ShadowHeight = 0;
	UINT m_LightType = LIGHT_OFF;

	Texture m_SpotLightShadowBuffer;
	Texture m_PointLightShadowBuffer;
	Texture m_DirectionalLightShadowBuffer;
	ConstantBuffer m_pShadowConstantsBuffers[6]; // spot, point, direc => 0, 6, 4개씩 사용.
	ConstantBuffer m_ShadowConstantsBufferForGS; // 2개 이상의 view 행렬을 사용하는 광원을 위한  geometry용 상수버퍼.
	
	// DO NOT release directly.
	BaseRenderer* m_pRenderer = nullptr;
};