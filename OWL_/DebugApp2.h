#pragma once

#include "Core/BaseRenderer.h"
#include "Geometry/Model.h"
#include "Geometry/SkinnedMeshModel.h"

class DebugApp2 : public Core::BaseRenderer
{
public:
	DebugApp2() : Core::BaseRenderer() { }
	~DebugApp2();

	void InitScene() override;

	// inline void UpdateLights(float deltaTime) override { Core::BaseRenderer::UpdateLights(deltaTime); }
	void UpdateGUI() override;
	void Update(float deltaTime) override;

	void Render() override;

private:
	Geometry::Model* m_pGround = nullptr;
	Geometry::SkinnedMeshModel* m_pCharacter = nullptr;
};