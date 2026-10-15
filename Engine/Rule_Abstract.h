#pragma once
#include <cstdint>
#include <string>
#include "WordsList.h"

namespace WordJewelsEngine
{
	
class Rule_Abstract
{
public:
	Rule_Abstract();
	Rule_Abstract(const Rule_Abstract& rhs);
	virtual ~Rule_Abstract();

public:
	virtual Rule_Abstract* Clone() const = 0;
	virtual const std::wstring RuleName() const = 0;
	virtual const int64_t RulePriority() const = 0;

public:
	enum RuleResult
	{
		Keep,
		Drop
	};

public:
	virtual RuleResult operator()(const WordsList::value_type& metaword) = 0;
	virtual RuleResult Run(const WordsList::value_type& metaword) = 0;
};

};
// namespace WordJewelsEngine
