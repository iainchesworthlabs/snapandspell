#pragma once
#ifndef _WITHFINALLY_H
#define _WITHFINALLY_H

#include <functional>

class WithFinally
{
public:
	typedef std::function<void()> CodeBlock;

public:
	WithFinally(const CodeBlock& f);
	~WithFinally();

public:
	static void Run(const CodeBlock& c, const CodeBlock& f);

private:
	CodeBlock m_Func;
};

#endif // _WITHFINALLY_H
