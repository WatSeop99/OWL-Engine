#pragma once

class BaseRenderer;
class Model;
class Scene;
class SkinnedMeshModel;

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
	Scene* m_pScene = nullptr;
	Model* m_pGround = nullptr;
	SkinnedMeshModel* m_pCharacter = nullptr;
};