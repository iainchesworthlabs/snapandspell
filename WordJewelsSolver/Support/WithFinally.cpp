#include "Stdafx.h"

#include <stdexcept>
#include "TracedThrow.h"
#include "WithFinally.h"

WithFinally::WithFinally(const WithFinally::CodeBlock& func) : 
	m_Func(func)
{
	if (!(func != nullptr))
	{
		Diagnostics::TracedThrow(std::invalid_argument("func != nullptr"), __FILE__, __LINE__, TEXT("std::invalid_argument"));
	}
}

WithFinally::~WithFinally() 
{ 
	m_Func();
}

void WithFinally::Run(const WithFinally::CodeBlock& c, const WithFinally::CodeBlock& f)
{
	WithFinally run(f);
	c();
}
