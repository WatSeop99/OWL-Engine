#pragma once

#include "../Common.h"
#include "GraphicsUtils.h"

namespace Core
{
	using std::vector;

	template <typename DATA_TYPE>
	class StructuredBuffer
	{
	public:
		StructuredBuffer() = default;
		virtual ~StructuredBuffer() { destroy(); }

		virtual void Initialize(ID3D11Device* pDevice, const UINT NUM_ELEMENTS)
		{
			CPU.resize(NUM_ELEMENTS);
			Initialize(pDevice);
		}
		virtual void Initialize(ID3D11Device* pDevice)
		{
			HRESULT hr = S_OK;

			hr = Graphics::CreateStructuredBuffer(pDevice,
												  (UINT)(CPU.size()), sizeof(DATA_TYPE), CPU.data(),
												  &pGPU, &pSRV, &pUAV);
			BREAK_IF_FAILED(hr);
			SET_DEBUG_INFO_TO_OBJECT(pGPU, "StructuredBuffer::pGPU");
			SET_DEBUG_INFO_TO_OBJECT(pSRV, "StructuredBuffer::pSRV");
			SET_DEBUG_INFO_TO_OBJECT(pUAV, "StructuredBuffer::pUAV");

			// Staging은 주로 디버깅 용도.
			hr = Graphics::CreateStagingBuffer(pDevice,
											   (UINT)(CPU.size()), sizeof(DATA_TYPE), nullptr,
											   &pStaging);
			BREAK_IF_FAILED(hr);
			SET_DEBUG_INFO_TO_OBJECT(pStaging, "StructuredBuffer::pStaging");
		}

		void Upload(ID3D11DeviceContext* pContext)
		{
			Upload(pContext, CPU);
		}
		void Upload(ID3D11DeviceContext* pContext, std::vector<DATA_TYPE>& arrCPU)
		{
			_ASSERT(arrCPU.size() == CPU.size());

			Graphics::CopyToStagingBuffer(pContext, pStaging, (UINT)(arrCPU.size() * sizeof(DATA_TYPE)), arrCPU.data());
			pContext->CopyResource(pGPU, pStaging);
		}

		void Download(ID3D11DeviceContext* pContext)
		{
			Download(pContext, CPU);
		}
		void Download(ID3D11DeviceContext* pContext, std::vector<DATA_TYPE>& arrCPU)
		{
			_ASSERT(arrCPU.size() == CPU.size());

			pContext->CopyResource(pStaging, pGPU);
			Graphics::CopyFromStagingBuffer(pContext, pStaging, (UINT)(arrCPU.size() * sizeof(DATA_TYPE)), arrCPU.data());
		}

	protected:
		void destroy()
		{
			SAFE_RELEASE(pGPU);
			SAFE_RELEASE(pStaging);
			SAFE_RELEASE(pSRV);
			SAFE_RELEASE(pUAV);
			CPU.clear();
		}

	public:
		std::vector<DATA_TYPE> CPU;
		ID3D11Buffer* pGPU = nullptr;
		ID3D11Buffer* pStaging = nullptr;

		ID3D11ShaderResourceView* pSRV = nullptr;
		ID3D11UnorderedAccessView* pUAV = nullptr;
	};

	// StructuredBuffer 대신 AppendBuffer 사용할 수도 있음.
	template <typename DATA_TYPE>
	class AppendBuffer : public StructuredBuffer<DATA_TYPE>
	{
		typedef StructuredBuffer<DATA_TYPE> BaseClass;

	public:
		~AppendBuffer() = default;

		void Initialize(ID3D11Device* pDevice)
		{
			Graphics::CreateAppendBuffer(pDevice, UINT(BaseClass::CPU.size()), sizeof(DATA_TYPE), BaseClass::CPU.data(),
										 &BaseClass::pGPU, &BaseClass::pSRV, &BaseClass::pUAV);
		}

		friend void swap(AppendBuffer<DATA_TYPE>& lhs, AppendBuffer<DATA_TYPE>& rhs)
		{
			std::swap(lhs.CPU, rhs.CPU);
			std::swap(lhs.pGPU, rhs.pGPU);
			std::swap(lhs.pSRV, rhs.pSRV);
			std::swap(lhs.pUAV, rhs.pUAV);
		}
	};

}
