#pragma once

#include "../Renderer/ConstantBuffer.h"
#include "../Graphics/ConstantDataType.h"
#include "../Renderer/Texture.h"

struct LightProperty;
class Camera;
class GraphicsPSO;
class Model;

class ShadowMap
{
public:
	ShadowMap(UINT width = 1280, UINT height = 1280) : m_ShadowWidth(width), m_ShadowHeight(height), m_LightType(LIGHT_OFF) {}
	~ShadowMap() { Cleanup(); }

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, UINT lightType);

	void Update(const LightProperty& PROPERTY, Camera& lightCam, Camera& mainCamera);

	void Render(std::vector<Model*>& pBasicList, Model* pMirror);

	void Cleanup();

	inline UINT GetShadowWidth() { return m_ShadowWidth; }
	inline UINT GetShadowHeight() { return m_ShadowHeight; }

	inline Texture* GetSpotLightShadowBufferPtr() { return &m_SpotLightShadowBuffer; }
	inline Texture* GetPointLightShadowBufferPtr() { return &m_PointLightShadowBuffer; }
	inline Texture* GetDirectionalLightShadowBufferPtr() { return &m_DirectionalLightShadowBuffer; }

	inline ConstantBuffer* GetShadowConstantBuffersPtr() { return m_pShadowConstantsBuffers; }

	inline void SetShadowWidth(const UINT WIDTH) { m_ShadowWidth = WIDTH; }
	inline void SetShadowHeight(const UINT HEIGHT) { m_ShadowHeight = HEIGHT; }

protected:
	void setPipelineState(const GraphicsPSO& PSO);
	void setShadowViewport();

	void calculateCascadeLightViewProjection(Vector3* pPosition, Matrix* pView, Matrix* pProjection, const Matrix& VIEW, const Matrix& PROJECTION, const Vector3& DIR, int cascadeIndex);

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;

	UINT m_ShadowWidth = 0;
	UINT m_ShadowHeight = 0;
	UINT m_LightType = LIGHT_OFF;

	Texture m_SpotLightShadowBuffer;
	Texture m_PointLightShadowBuffer;
	Texture m_DirectionalLightShadowBuffer;
	ConstantBuffer m_pShadowConstantsBuffers[6]; // spot, point, direc => 0, 6, 4개씩 사용.
	ConstantBuffer m_ShadowConstantsBufferForGS; // 2개 이상의 view 행렬을 사용하는 광원을 위한  geometry용 상수버퍼.
};