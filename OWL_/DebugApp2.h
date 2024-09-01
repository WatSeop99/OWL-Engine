#pragma once

#include "Renderer/BaseRenderer.h"
#include "Geometry/Model.h"
#include "Geometry/SkinnedMeshModel.h"

class DebugApp2 : public BaseRenderer
{
public:
	DebugApp2() = default;
	~DebugApp2();

	void InitScene() override;

	// inline void UpdateLights(float deltaTime) override { Core::BaseRenderer::UpdateLights(deltaTime); }
	void UpdateGUI() override;
	void Update(float deltaTime) override;

	void Render() override;

private:
	Model* m_pGround = nullptr;
	SkinnedMeshModel* m_pCharacter = nullptr;
};