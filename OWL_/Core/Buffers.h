#pragma once

#include "../Common.h"
#include "GraphicsUtils.h"

namespace Core
{
	template <typename DATA_TYPE>
	class StagingBuffer
	{
	public:
		~StagingBuffer() { destroy(); }

		void Initialize(ID3D11Device* pDevice, const std::vector<DATA_TYPE>& DATA)
		{
			HRESULT hr = S_OK;

			destroy();

			CPU = DATA;
			hr = Graphics::CreateStagingBuffer(pDevice, (UINT)(DATA.size()), sizeof(DATA_TYPE), DATA.data(), &pGPU);
			BREAK_IF_FAILED(hr);
			SET_DEBUG_INFO_TO_OBJECT(pGPU, "StagingBuffer::pGPU");
		}

		void Download(ID3D11DeviceContext* pContext)
		{
			D3D11_MAPPED_SUBRESOURCE mappedResource = { 0, };

			pContext->Map(pGPU, 0, D3D11_MAP_READ, 0, &mappedResource);
			uint8_t* pData = (uint8_t*)mappedResource.pData;
			memcpy(CPU.data(), &pData[0], sizeof(DATA_TYPE) * CPU.size());
			pContext->Unmap(pGPU, 0);
		}

	protected:
		void destroy()
		{
			SAFE_RELEASE(pGPU);
			CPU.clear();
		}

	public:
		std::vector<DATA_TYPE> CPU;
		ID3D11Buffer* pGPU = nullptr;
	};
}
