#pragma once

#include "ImageFilter.h"
#include "../Geometry/Mesh.h"

namespace Core
{
	class PostProcess
	{
	public:
		~PostProcess() { destroy(); }

		void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<ID3D11ShaderResourceView*>& RESOURCES, const std::vector<ID3D11RenderTargetView*>& TARGETS,
						const int WIDTH, const int HEIGHT, const int BLOOMLEVELS);

		void CreateBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, int width, int height, ID3D11ShaderResourceView** ppSrv, ID3D11RenderTargetView** ppRtv);

		void Render(ID3D11DeviceContext* pContext);
		void RenderImageFilter(ID3D11DeviceContext* pContext, const ImageFilter& IMAGE_FILTER);

	protected:
		void destroy();

	public:
		ImageFilter CombineFilter;
		std::vector<ImageFilter> pBloomDownFilters;
		std::vector<ImageFilter> pBloomUpFilters;

		struct Geometry::Mesh* pMesh = nullptr;

	private:
		std::vector<ID3D11ShaderResourceView*> m_pBloomSRVs;
		std::vector<ID3D11RenderTargetView*> m_pBloomRTVs;
	};
}