#pragma once
#ifndef _SINGLETON_H
#define _SINGLETON_H

#include <atomic>
#include <memory>
#include <mutex>

template<typename DerivedType>
class Singleton
{
protected:
	Singleton() 
	{
	}

public:
	~Singleton() 
	{ 
	}

	static DerivedType& GetInstance()
	{
		std::call_once(Singleton::m_Once, []()
		{ 
			Singleton::m_Instance.reset(new TDerived()); 
		});

		return *m_Instance;
	}

private:
	static std::unique_ptr<DerivedType> m_Instance;
	static std::once_flag               m_Once;
};

template<typename DerivedType> std::unique_ptr<DerivedType> Singleton<DerivedType>::m_Instance;
template<typename DerivedType> std::once_flag Singleton<DerivedType>::m_Once;

#endif // _SINGLETON_H
