#pragma once

#include "Camera.h"
#include "ShadowMap.h"

class Model;

class Light
{
public:
	Light(UINT width = 1280, UINT height = 1280);
	~Light() { Cleanup(); }

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

	void Update(float deltaTime, Camera& mainCamera);

	void RenderShadowMap(std::vector<Model*>& pBasicList, Model* pMirror);

	void Cleanup();

	inline ShadowMap& GetShadowMap() { return m_ShadowMap; }
	inline ShadowMap* GetAddressOfShadowMap() { return &m_ShadowMap; }

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
