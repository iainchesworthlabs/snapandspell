#include "Stdafx.h"
#include "Rule_ThreeLettersOrMore.h"

namespace WordJewelsEngine
{

const std::wstring Rule_ThreeLettersOrMore::RULE_NAME(L"ThreeLettersOrMore");
const int64_t Rule_ThreeLettersOrMore::RULE_DEFAULT_PRIORITY(127);

Rule_ThreeLettersOrMore::Rule_ThreeLettersOrMore() : Rule_Abstract()
{
}

Rule_ThreeLettersOrMore::Rule_ThreeLettersOrMore(const Rule_ThreeLettersOrMore& rhs) : Rule_Abstract()
{
}

Rule_ThreeLettersOrMore::~Rule_ThreeLettersOrMore()
{
}

Rule_Abstract* Rule_ThreeLettersOrMore::Clone() const
{
	return new Rule_ThreeLettersOrMore(*this);
}

const std::wstring Rule_ThreeLettersOrMore::RuleName() const
{
	return RULE_NAME;
}

const int64_t Rule_ThreeLettersOrMore::RulePriority() const
{
	return RULE_DEFAULT_PRIORITY;
}

Rule_ThreeLettersOrMore::RuleResult Rule_ThreeLettersOrMore::operator()(const WordsList::value_type& metaword)
{
	return Run(metaword);
}

Rule_ThreeLettersOrMore::RuleResult Rule_ThreeLettersOrMore::Run(const WordsList::value_type& metaword)
{
	static const uint32_t MINIMUM_LENGTH = 3;
	return (MINIMUM_LENGTH <= std::get<0>(metaword).length()) ? Keep : Drop;
}

};
// namespace WordJewelsEngine