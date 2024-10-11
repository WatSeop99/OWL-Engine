#pragma once

#include "Renderer/BaseRenderer.h"
#include "Geometry/Model.h"
#include "Geometry/SkinnedMeshModel.h"

//class DebugApp2 : public BaseRenderer
//{
//public:
//	DebugApp2() = default;
//	~DebugApp2();
//
//	void InitScene() override;
//	
//	void UpdateGUI() override;
//	void Update(float deltaTime) override;
//
//	void Render() override;
//
//private:
//	Model* m_pGround = nullptr;
//	SkinnedMeshModel* m_pCharacter = nullptr;
//};
class DebugApp2
{
public:
	DebugApp2() = default;
	~DebugApp2();

	int Run();

	void Initialize();
	void InitScene();

	void UpdateGUI();
	void Update(float deltaTime);

	void Render();

private:
	BaseRenderer* m_pRenderer = nullptr;
	Model* m_pGround = nullptr;
	SkinnedMeshModel* m_pCharacter = nullptr;
};