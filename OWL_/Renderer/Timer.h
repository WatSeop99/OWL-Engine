#pragma once

#include <chrono>

class Timer
{
public:
	Timer() = default;
	~Timer() { Cleanup(); }

	void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

	void Start(bool bMeasureGPU);
	
	void End();

	void Cleanup();

public:
	double ElapsedTimeCPU = 0.0f;
	double ElapsedTimeGPU = 0.0f;

protected:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;

	ID3D11Query* m_pStartQuery = nullptr;
	ID3D11Query* m_pStopQuery = nullptr;
	ID3D11Query* m_pDisjointQuery = nullptr;

	std::chrono::steady_clock::time_point m_StartTimeCPU;

	bool m_bMeasureGPU = false;
};
