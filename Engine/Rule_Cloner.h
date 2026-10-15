#pragma once
#include "Rule_Abstract.h"

namespace WordJewelsEngine
{

class Rule_Cloner
{
	Rule_Abstract* m_Ptr; // Ptr is mandatory to achieve polymorphic behavior
	Rule_Cloner& operator=(const Rule_Cloner&);

public:
	template<typename T> Rule_Cloner(const T& t) : m_Ptr(new T(t)) 
	{
	}

	Rule_Cloner(const Rule_Cloner& rhs) : m_Ptr (rhs.m_Ptr->Clone())
	{
	}

	virtual ~Rule_Cloner() 
	{ 
		if (nullptr != m_Ptr)
		{
			delete m_Ptr; 
		}
	}

	Rule_Abstract* operator->() { return m_Ptr; }
    Rule_Abstract& operator*() { return *m_Ptr; }
};

};
// namespace WordJewelsEngine
