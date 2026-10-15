#pragma once
#ifndef _ACTIONTIMER_H
#define _ACTIONTIMER_H

#include <chrono>

namespace Diagnostics
{

class ActionTimer
{
private:
	typedef std::chrono::high_resolution_clock clock;
	typedef std::chrono::microseconds microseconds;
	typedef std::chrono::milliseconds milliseconds;

public:
	ActionTimer();
	ActionTimer(const ActionTimer& RHS);
	ActionTimer& operator=(const ActionTimer& RHS);

public:
	inline void Start()
	{
		m_Start = now();
		m_IsRunning = true;
	}

	inline void Stop()
	{
		m_Stop = now();
		m_IsRunning = false;
	}

	void Reset();
	void Restart();
	bool IsRunning() const;

	microseconds ElapsedInUs() const;
	milliseconds ElapsedInMs() const;

private:
	clock::time_point now() const;
	microseconds IntervalInUs(const clock::time_point& t1, const clock::time_point& t0) const;
	milliseconds IntervalInMs(const clock::time_point& t1, const clock::time_point& t0) const;

private:
	clock::time_point m_Start;
	clock::time_point m_Stop;
	bool m_IsRunning;
};

}
// namespace Diagnostics

#endif // _ACTIONTIMER_H
