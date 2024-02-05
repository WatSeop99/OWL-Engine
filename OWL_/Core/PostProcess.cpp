#include "../Common.h"
#include "../Geometry/GeometryGenerator.h"
#include "GraphicsCommon.h"
#include "GraphicsUtils.h"
#include "../Geometry/MeshData.h"
#include "PostProcess.h"

namespace Core
{
	void PostProcess::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const std::vector<ID3D11ShaderResourceView*>& RESOURCES, const std::vector<ID3D11RenderTargetView*>& TARGETS,
								 const int WIDTH, const int HEIGHT, const int BLOOMLEVELS)
	{
		HRESULT hr = S_OK;

		destroy();

		struct Geometry::MeshData meshData;
		Geometry::MakeSquare(&meshData);
		pMesh = (struct Geometry::Mesh*)Malloc(sizeof(struct Geometry::Mesh));
		*pMesh = INIT_MESH;

		hr = Graphics::CreateVertexBuffer(pDevice, meshData.Vertices, &(pMesh->pVertexBuffer));
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(pMesh->pVertexBuffer, "PostProcess::pMesh->pVertexBuffer");

		pMesh->IndexCount = (UINT)(meshData.Indices.size());
		hr = Graphics::CreateIndexBuffer(pDevice, meshData.Indices, &(pMesh->pIndexBuffer));
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(pMesh->pIndexBuffer, "PostProcess::pMesh->pIndexBuffer");

		// Bloom Down/Up.
		m_pBloomSRVs.resize(BLOOMLEVELS);
		m_pBloomRTVs.resize(BLOOMLEVELS);
		for (int i = 0; i < BLOOMLEVELS; ++i)
		{
			int div = (int)pow(2, i);
			CreateBuffer(pDevice, pContext, WIDTH / div, HEIGHT / div, &m_pBloomSRVs[i], &m_pBloomRTVs[i]);
#ifdef _DEBUG
			std::string post(std::to_string(i) + "]");
			std::string debugStringSRV("PostProcess::m_pBloomSRVs[");
			std::string debugStringRTV("PostProcess::m_pBloomRTVs[");
			debugStringSRV += post;
			debugStringRTV += post;
			SET_DEBUG_INFO_TO_OBJECT(m_pBloomSRVs[i], debugStringSRV.c_str());
			SET_DEBUG_INFO_TO_OBJECT(m_pBloomRTVs[i], debugStringRTV.c_str());
#endif
		}

		pBloomDownFilters.resize(BLOOMLEVELS - 1);
		for (int i = 0; i < BLOOMLEVELS - 1; ++i)
		{
			int div = (int)pow(2, i + 1);
			pBloomDownFilters[i].Initialize(pDevice, pContext, Graphics::g_pBloomDownPS, WIDTH / div, HEIGHT / div);
			if (i == 0)
			{
				pBloomDownFilters[i].SetShaderResources({ RESOURCES[0] });
			}
			else
			{
				pBloomDownFilters[i].SetShaderResources({ m_pBloomSRVs[i] });
			}

			pBloomDownFilters[i].SetRenderTargets({ m_pBloomRTVs[i + 1] });
		}

		pBloomUpFilters.resize(BLOOMLEVELS - 1);
		for (int i = 0; i < BLOOMLEVELS - 1; ++i)
		{
			int level = BLOOMLEVELS - 2 - i;
			int div = (int)pow(2, level);
			pBloomUpFilters[i].Initialize(pDevice, pContext, Graphics::g_pBloomUpPS, WIDTH / div, HEIGHT / div);
			pBloomUpFilters[i].SetShaderResources({ m_pBloomSRVs[level + 1] });
			pBloomUpFilters[i].SetRenderTargets({ m_pBloomRTVs[level] });
		}

		// combine + tone mapping.
		CombineFilter.Initialize(pDevice, pContext, Graphics::g_pCombinePS, WIDTH, HEIGHT);
		CombineFilter.SetShaderResources({ RESOURCES[0], m_pBloomSRVs[0], RESOURCES[1] }); // resource[1]은 모션 블러를 위한 이전 프레임 결과.
		CombineFilter.SetRenderTargets(TARGETS);
		CombineFilter.ConstantsData.Strength = 0.0f; // bloom strength.
		CombineFilter.ConstantsData.Option1 = 1.0f;  // exposure.
		CombineFilter.ConstantsData.Option2 = 2.2f;  // gamma.

		// 주의: float render target에서는 gamma correction 하지 않음. (gamma = 1.0)
		CombineFilter.UpdateConstantBuffers(pContext);
	}

	void PostProcess::CreateBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, int width, int height, ID3D11ShaderResourceView** ppSrv, ID3D11RenderTargetView** ppRtv)
	{
		HRESULT hr = S_OK;

		ID3D11Texture2D* texture = nullptr;

		D3D11_TEXTURE2D_DESC txtDesc = { 0, };
		txtDesc.Width = width;
		txtDesc.Height = height;
		txtDesc.MipLevels = txtDesc.ArraySize = 1;
		txtDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // 이미지 처리용도.
		txtDesc.SampleDesc.Count = 1;
		txtDesc.Usage = D3D11_USAGE_DEFAULT;
		txtDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		txtDesc.MiscFlags = 0;
		txtDesc.CPUAccessFlags = 0;

		hr = pDevice->CreateTexture2D(&txtDesc, nullptr, &texture);
		BREAK_IF_FAILED(hr);
		SET_DEBUG_INFO_TO_OBJECT(texture, "PostProcess::texture");

		hr = pDevice->CreateRenderTargetView(texture, nullptr, ppRtv);
		BREAK_IF_FAILED(hr);

		hr = pDevice->CreateShaderResourceView(texture, nullptr, ppSrv);
		BREAK_IF_FAILED(hr);

		RELEASE(texture);
	}

	void PostProcess::Render(ID3D11DeviceContext* pContext)
	{
		pContext->PSSetSamplers(0, 1, &Graphics::g_pLinearClampSS);

		UINT stride = sizeof(struct Geometry::Vertex);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &(pMesh->pVertexBuffer), &stride, &offset);
		pContext->IASetIndexBuffer(pMesh->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// 블룸이 필요한 경우에만 계산.
		if (CombineFilter.ConstantsData.Strength > 0.0f)
		{
			for (size_t i = 0, size = pBloomDownFilters.size(); i < size; ++i)
			{
				RenderImageFilter(pContext, pBloomDownFilters[i]);
			}
			for (size_t i = 0, size = pBloomUpFilters.size(); i < size; ++i)
			{
				RenderImageFilter(pContext, pBloomUpFilters[i]);
			}
		}

		RenderImageFilter(pContext, CombineFilter);
	}

	void PostProcess::RenderImageFilter(ID3D11DeviceContext* pContext, const ImageFilter& IMAGE_FILTER)
	{
		IMAGE_FILTER.Render(pContext);
		pContext->DrawIndexed(pMesh->IndexCount, 0, 0);
	}

	void PostProcess::destroy()
	{
		for (size_t i = 0, size = m_pBloomSRVs.size(); i < size; ++i)
		{
			RELEASE(m_pBloomSRVs[i]);
			RELEASE(m_pBloomRTVs[i]);
		}
		m_pBloomSRVs.clear();
		m_pBloomRTVs.clear();

		if (pMesh)
		{
			ReleaseMesh(&pMesh);
		}
		pBloomDownFilters.clear();
		pBloomUpFilters.clear();
	}

}