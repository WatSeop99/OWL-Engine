#pragma once

#include "Camera.h"
#include "ConstantBuffers.h"
#include "../Geometry/Model.h"
#include "ShadowMap.h"

class Light
{
public:
	Light(UINT width = 1280, UINT height = 1280);
	~Light() { Cleanup(); }

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

	void Update(ID3D11DeviceContext* pContext, float deltaTime, Camera& mainCamera);

	void RenderShadowMap(ID3D11DeviceContext* pContext, std::vector<Model*>& pBasicList, Model* pMirror);

	void Cleanup();

	inline ShadowMap& GetShadowMap() { return m_ShadowMap; }
	inline ShadowMap* GetAddressOfShadowMap() { return &m_ShadowMap; }

	inline void SetPosition(const Vector3& pos) { m_LightViewCamera.SetEyePos(pos); }
	inline void SetDirection(const Vector3& dir) { m_LightViewCamera.SetViewDir(dir); }
	inline void SetShadowSize(const UINT WIDTH, const UINT HEIGHT) { m_ShadowMap.SetShadowWidth(WIDTH); m_ShadowMap.SetShadowHeight(HEIGHT); }

public:
	bool bRotated = false;
	bool bVisible = true;
	LightProperty Property;

private:
	Camera m_LightViewCamera;
	ShadowMap m_ShadowMap;
};
