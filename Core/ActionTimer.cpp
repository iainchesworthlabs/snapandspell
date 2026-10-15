#include "pch.h"
#include "ActionTimer.h"

namespace Diagnostics
{

ActionTimer::ActionTimer() :
	m_Start(now()),
	m_Stop(now()),
	m_IsRunning(false)
{
}

ActionTimer::ActionTimer(const ActionTimer& RHS) :
	m_Start(RHS.m_Start),
	m_Stop(RHS.m_Stop),
	m_IsRunning(RHS.m_IsRunning)
{
}

ActionTimer& ActionTimer::operator = (const ActionTimer& RHS)
{
	if (&RHS != this)
	{
		m_Start = RHS.m_Start;
		m_Stop = RHS.m_Stop;
		m_IsRunning = RHS.m_IsRunning;
	}

	return *this;
}

void ActionTimer::Reset()
{
	m_Start = now();
	m_Stop = now();
	m_IsRunning = false;
}

void ActionTimer::Restart()
{
	m_Start = now();
	m_Stop = now();
	m_IsRunning = true;
}

bool ActionTimer::IsRunning() const
{
	return m_IsRunning;
}

ActionTimer::microseconds ActionTimer::ElapsedInUs() const
{
	return IntervalInUs(m_Stop, m_Start);
}

ActionTimer::milliseconds ActionTimer::ElapsedInMs() const
{
	return IntervalInMs(m_Stop, m_Start);
}

ActionTimer::clock::time_point ActionTimer::now() const
{
	return clock::now();
}

ActionTimer::microseconds ActionTimer::IntervalInUs(const clock::time_point& t1, const clock::time_point& t0) const
{
	return std::chrono::duration_cast<microseconds>(t1 - t0);
}

ActionTimer::milliseconds ActionTimer::IntervalInMs(const clock::time_point& t1, const clock::time_point& t0) const
{
	return std::chrono::duration_cast<milliseconds>(t1 - t0);
}

}
// namespace Diagnostics
