#pragma once

#include "Camera.h"
#include "ShadowMap.h"

class BaseRenderer;
class Model;

class Light
{
private:
	const UINT m_TOTAL_LIGHT_TYPE = (LIGHT_DIRECTIONAL | LIGHT_POINT | LIGHT_SPOT);

public:
	Light(UINT width = 1280, UINT height = 1280);
	~Light() { Cleanup(); }

	void Initialize(BaseRenderer* pRenderer);

	void Update(float deltaTime, Camera* pMainCamera);

	void RenderShadowMap(std::vector<Model*>& pBasicList, Model* pMirror);

	void Cleanup();

	inline ShadowMap& GetShadowMap() { return m_ShadowMap; }
	inline ShadowMap* GetShadowMapPtr() { return &m_ShadowMap; }

	inline void SetPosition(const Vector3& POS) { m_LightViewCamera.SetEyePos(POS); }
	inline void SetDirection(const Vector3& DIR) { m_LightViewCamera.SetViewDir(DIR); }
	inline void SetShadowSize(const UINT WIDTH, const UINT HEIGHT) { m_ShadowMap.SetShadowWidth(WIDTH); m_ShadowMap.SetShadowHeight(HEIGHT); }

public:
	bool bRotated = false;
	bool bVisible = true;
	LightProperty Property;
	
private:
	Camera m_LightViewCamera;
	ShadowMap m_ShadowMap;
};
