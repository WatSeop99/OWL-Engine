#pragma once

#include <chrono>


class Timer
{
public:
	Timer() : ElapsedTimeCPU(0.0f), ElapsedTimeGPU(0.0f), m_bMeasureGPU(false) {}
	Timer(ID3D11Device* pDevice);
	~Timer() { destroy(); }

	void Start(ID3D11DeviceContext* pContext, bool bMeasureGPU);
	void End(ID3D11DeviceContext* pContext);

protected:
	void destroy();

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
