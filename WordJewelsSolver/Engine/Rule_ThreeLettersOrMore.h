#pragma once
#include <cstdint>
#include <string>
#include "Rule_Abstract.h"
#include "WordsList.h"

namespace WordJewelsEngine
{

class Rule_ThreeLettersOrMore : public Rule_Abstract
{
public:
	Rule_ThreeLettersOrMore();
	Rule_ThreeLettersOrMore(const Rule_ThreeLettersOrMore& rhs);
	virtual ~Rule_ThreeLettersOrMore();

public:
	virtual Rule_Abstract* Clone() const;
	virtual const std::wstring RuleName() const;
	virtual const int64_t RulePriority() const;

public:
	virtual RuleResult operator()(const WordsList::value_type& metaword);
	virtual RuleResult Run(const WordsList::value_type& metaword);

public:
	static const std::wstring RULE_NAME;
	static const int64_t RULE_DEFAULT_PRIORITY;
};

};
// namespace WordJewelsEngine