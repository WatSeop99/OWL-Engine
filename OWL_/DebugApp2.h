#pragma once

class Renderer;
class Model;
class Scene;
class SkinnedMeshModel;

class DebugApp2
{
public:
	DebugApp2() = default;
	~DebugApp2();

	int Run();

	bool Initialize(HINSTANCE hInstance);
	void InitScene();

	void UpdateGUI();
	void Update(const float DELTA_TIME);

	void Render();

private:
	Renderer* m_pRenderer = nullptr;
	Scene* m_pScene = nullptr;
	Model* m_pGround = nullptr;
	SkinnedMeshModel* m_pCharacter = nullptr;
};