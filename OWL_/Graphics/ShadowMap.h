#pragma once

#include "../Renderer/ConstantBuffer.h"
#include "../Graphics/ConstantDataType.h"
#include "../Renderer/Texture.h"

struct LightProperty;
class Renderer;
class Camera;
class GraphicsPSO;
class Model;

class ShadowMap
{
private:
	const UINT m_TOTAL_LIGHT_TYPE = (LIGHT_DIRECTIONAL | LIGHT_POINT | LIGHT_SPOT | LIGHT_SUN);

public:
	ShadowMap(UINT width = 1280, UINT height = 1280) : m_ShadowWidth(width), m_ShadowHeight(height) {}
	~ShadowMap() { Cleanup(); }

	void Initialize(Renderer* pRenderer, UINT lightType);

	void Update(const LightProperty& PROPERTY, Camera* pLightCam, Camera* pMainCamera);

	void Render(std::vector<Model*>& pBasicList, Model* pMirror);

	void Cleanup();

	inline UINT GetShadowWidth() { return m_ShadowWidth; }
	inline UINT GetShadowHeight() { return m_ShadowHeight; }

	inline Texture* GetShadow2DBufferPtr() { return &m_Shadow2DBuffer; }
	inline Texture* GetShadowCubeBufferPtr() { return &m_ShadowCubeBuffer; }
	inline Texture* GetCascadeShadowBufferPtr() { return &m_CascadeShadowBuffer; }

	inline ConstantBuffer* GetShadowConstantBuffers() { return m_pShadowConstantsBuffers; }

	inline void SetShadowWidth(UINT width) { m_ShadowWidth = width; }
	inline void SetShadowHeight(UINT height) { m_ShadowHeight = height; }

protected:
	void SetShadowViewport();

	void CalculateCascadeLightViewProjection(Vector3* pPosition, Matrix* pView, Matrix* pProjection, const Matrix& VIEW, const Matrix& PROJECTION, const Vector3& DIR, int cascadeIndex);

private:
	UINT m_ShadowWidth = 0;
	UINT m_ShadowHeight = 0;
	UINT m_LightType = LIGHT_OFF;

	Texture m_Shadow2DBuffer;
	Texture m_ShadowCubeBuffer;
	Texture m_CascadeShadowBuffer;
	ConstantBuffer m_pShadowConstantsBuffers[6]; // spot, point, direc => 0, 6, 4개씩 사용.
	ConstantBuffer m_ShadowConstantsBufferForGS; // 2개 이상의 view 행렬을 사용하는 광원을 위한  geometry용 상수버퍼.
	
	// DO NOT release directly.
	Renderer* m_pRenderer = nullptr;
};