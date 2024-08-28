#pragma once

#include "Renderer/BaseRenderer.h"
#include "Geometry/Model.h"


using DirectX::BoundingSphere;
using DirectX::SimpleMath::Vector3;

class DebugApp : public BaseRenderer
{
public:
	DebugApp();
	~DebugApp();

	void InitScene() override;

	// inline void UpdateLights(float deltaTime) { BaseRenderer::UpdateLights(deltaTime); }
	void UpdateGUI() override;
	inline void Update(float deltaTime) override { BaseRenderer::Update(deltaTime); }

	void Render() override;

protected:
	Model* m_pGround = nullptr;
};


