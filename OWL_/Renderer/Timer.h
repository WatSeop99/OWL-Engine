#pragma once

#include <chrono>

class Timer
{
public:
	Timer() = default;
	Timer(ID3D11Device* pDevice);
	~Timer() { Cleanup(); }

	void Start(ID3D11DeviceContext* pContext, bool bMeasureGPU);
	
	void End(ID3D11DeviceContext* pContext);

	void Cleanup();

public:
	double ElapsedTimeCPU = 0.0f;
	double ElapsedTimeGPU = 0.0f;

protected:
	ID3D11Query* m_pStartQuery = nullptr;
	ID3D11Query* m_pStopQuery = nullptr;
	ID3D11Query* m_pDisjointQuery = nullptr;

	std::chrono::steady_clock::time_point m_StartTimeCPU;

	bool m_bMeasureGPU = false;
};
