
#include "../Common.h"
#include "Timer.h"


Timer::Timer(ID3D11Device* pDevice) :
	ElapsedTimeCPU(0.0f),
	ElapsedTimeGPU(0.0f),
	m_bMeasureGPU(false)
{
	_ASSERT(pDevice != nullptr);

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

void Timer::Start(ID3D11DeviceContext* pContext, bool bMeasureGPU)
{
	_ASSERT(pContext != nullptr);

	m_bMeasureGPU = bMeasureGPU;

	ElapsedTimeGPU = 0.0f;
	ElapsedTimeCPU = 0.0f;

	m_StartTimeCPU = std::chrono::high_resolution_clock::now();

	if (m_bMeasureGPU)
	{
		pContext->Begin(m_pDisjointQuery);
		pContext->End(m_pStartQuery);
	}
}

void Timer::End(ID3D11DeviceContext* pContext)
{
	_ASSERT(pContext != nullptr);

	if (m_bMeasureGPU)
	{
		// GPU Profiling in DX11 with Queries.
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

		if (tsDisjoint.Disjoint == FALSE)
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
		OutputDebugStringA(" milliSec, ");
	}

	sprintf(debugString, "%lf", ElapsedTimeCPU);

	OutputDebugStringA("CPU: ");
	OutputDebugStringA(debugString);
	OutputDebugStringA(" milliSec\n");
}

void Timer::destroy()
{
	SAFE_RELEASE(m_pDisjointQuery);
	SAFE_RELEASE(m_pStopQuery);
	SAFE_RELEASE(m_pStartQuery);
}
