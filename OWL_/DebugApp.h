#pragma once

#include "Core/BaseRenderer.h"
#include "Geometry/Model.h"


using DirectX::BoundingSphere;
using DirectX::SimpleMath::Vector3;

class DebugApp : public Core::BaseRenderer
{
public:
	DebugApp();
	~DebugApp();

	void InitScene() override;

	inline void UpdateLights(float deltaTime) { BaseRenderer::UpdateLights(deltaTime); }
	void UpdateGUI() override;
	inline void Update(float deltaTime) override { BaseRenderer::Update(deltaTime); }

	void Render() override;

protected:
	Geometry::Model* m_pGround = nullptr;
};


