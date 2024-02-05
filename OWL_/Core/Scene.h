#pragma once

namespace Core
{
	class Scene
	{
	public:
		Scene() = default;
		~Scene() = default;

		void LoadScene();
		void SaveScene();


	public:
		std::vector<Geometry::Model*> pRenderObjects;
		std::vector<Light*> pLights;
		Light* pDirectionalLight = nullptr;
		Core::Camera& Camera;
	};
}
