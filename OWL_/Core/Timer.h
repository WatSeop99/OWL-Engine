#pragma once

#include "../Common.h"
#include <chrono>
#include <d3d11.h>
#include <iostream>

namespace Core
{
	class Timer
	{
	public:
		Timer() : 
			ElapsedTimeCPU(0.0f),
			ElapsedTimeGPU(0.0f),
			m_bMeasureGPU(false)
		{ }
		Timer(ID3D11Device* pDevice) :
			ElapsedTimeCPU(0.0f),
			ElapsedTimeGPU(0.0f),
			m_bMeasureGPU(false)
		{
			HRESULT hr = S_OK;

			D3D11_QUERY_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Query = D3D11_QUERY_TIMESTAMP;
			desc.MiscFlags = 0;
			hr = pDevice->CreateQuery(&desc, &m_pStartQuery);
			BREAK_IF_FAILED(hr);
			SET_DEBUG_INFO_TO_OBJECT(m_pStartQuery, "Timer::m_pStartQuery");

			hr = pDevice->CreateQuery(&desc, &m_pStopQuery);
			BREAK_IF_FAILED(hr);
			SET_DEBUG_INFO_TO_OBJECT(m_pStopQuery, "Timer::m_pStopQuery");

			desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
			hr = pDevice->CreateQuery(&desc, &m_pDisjointQuery);
			BREAK_IF_FAILED(hr);
			SET_DEBUG_INFO_TO_OBJECT(m_pDisjointQuery, "Timer::m_pDisjointQuery");
		}
		~Timer()
		{
			SAFE_RELEASE(m_pDisjointQuery);
			SAFE_RELEASE(m_pStopQuery);
			SAFE_RELEASE(m_pStartQuery);
		}

		void Start(ID3D11DeviceContext* pContext, bool bMeasureGPU)
		{
			m_bMeasureGPU = bMeasureGPU;

			double ElapsedTimeGPU = 0.0f;
			double ElapsedTimeCPU = 0.0f;

			m_StartTimeCPU = std::chrono::high_resolution_clock::now();

			if (m_bMeasureGPU)
			{
				pContext->Begin(m_pDisjointQuery);
				pContext->End(m_pStartQuery);
			}
		}

		void End(ID3D11DeviceContext* pContext)
		{
			if (m_bMeasureGPU)
			{
				// GPU Profiling in DX11 with Queries
				// https://therealmjp.github.io/posts/profiling-in-dx11-with-queries/

				pContext->End(m_pStopQuery);
				pContext->End(m_pDisjointQuery);

				while (pContext->GetData(m_pDisjointQuery, nullptr, 0, 0) == S_FALSE)
				{
					continue;
				}
				D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
				pContext->GetData(m_pDisjointQuery, &tsDisjoint, sizeof(tsDisjoint), 0);

				// The timestamp returned by ID3D11DeviceContext::GetData for a
				// timestamp query is only reliable if Disjoint is FALSE.
				// https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_query_data_timestamp_disjoint

				if (!tsDisjoint.Disjoint)
				{
					UINT64 startTime, stopTime;
					pContext->GetData(m_pStartQuery, &startTime, sizeof(UINT64), 0);
					pContext->GetData(m_pStopQuery, &stopTime, sizeof(UINT64), 0);

					ElapsedTimeGPU = ((double)(stopTime - startTime) / (double)(tsDisjoint.Frequency)) * 1000.0;
				}
				else
				{
					ElapsedTimeGPU = -1.0f;
				}
			}

			ElapsedTimeCPU = (std::chrono::high_resolution_clock::now() - m_StartTimeCPU).count() / double(1e6); // microsec -> millisec

			char debugString[256] = { 0, };
			if (m_bMeasureGPU)
			{
				sprintf(debugString, "%lf", ElapsedTimeGPU);

				OutputDebugStringA("GPU: ");
				OutputDebugStringA(debugString);
				OutputDebugStringA(", ");
			}

			sprintf(debugString, "%lf", ElapsedTimeCPU);

			OutputDebugStringA("CPU: ");
			OutputDebugStringA(debugString);
			OutputDebugStringA("\n");
		}

	public:
		double ElapsedTimeCPU;
		double ElapsedTimeGPU;

	protected:
		ID3D11Query* m_pStartQuery = nullptr;
		ID3D11Query* m_pStopQuery = nullptr;
		ID3D11Query* m_pDisjointQuery = nullptr;

		std::chrono::steady_clock::time_point m_StartTimeCPU;

		bool m_bMeasureGPU;
	};
}
