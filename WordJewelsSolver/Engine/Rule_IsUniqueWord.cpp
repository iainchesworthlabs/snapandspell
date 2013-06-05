#include "Stdafx.h"
#include "Rule_IsUniqueWord.h"

namespace WordJewelsEngine
{

const std::wstring Rule_IsUniqueWord::RULE_NAME(L"IsUniqueWord");
const int64_t Rule_IsUniqueWord::RULE_DEFAULT_PRIORITY(127);

Rule_IsUniqueWord::Rule_IsUniqueWord() : Rule_Abstract()
{
}

Rule_IsUniqueWord::Rule_IsUniqueWord(const Rule_IsUniqueWord& rhs) : Rule_Abstract()
{
}

Rule_IsUniqueWord::~Rule_IsUniqueWord()
{
}

Rule_Abstract* Rule_IsUniqueWord::Clone() const
{
	return new Rule_IsUniqueWord(*this);
}

const std::wstring Rule_IsUniqueWord::RuleName() const
{
	return RULE_NAME;
}

const int64_t Rule_IsUniqueWord::RulePriority() const
{
	return RULE_DEFAULT_PRIORITY;
}

Rule_IsUniqueWord::RuleResult Rule_IsUniqueWord::operator()(const WordsList::value_type& metaword)
{
	return Run(metaword);
}

Rule_IsUniqueWord::RuleResult Rule_IsUniqueWord::Run(const WordsList::value_type& metaword)
{
	return Keep;
}

};
// namespace WordJewelsEngine
