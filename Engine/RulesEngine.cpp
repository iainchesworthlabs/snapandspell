#include "pch.h"
#include <algorithm>
#include "RulesEngine.h"

namespace WordJewelsEngine
{

RulesEngine::RulesEngine() : m_Rules()
{
}

RulesEngine::~RulesEngine()
{
}

void RulesEngine::AddRule(Rule_Cloner rule, int64_t priority)
{
	m_Rules.push_back(std::make_pair(rule, priority));
}

void RulesEngine::Execute(WordsList& listOfWords) const
{
	std::for_each(m_Rules.cbegin(), m_Rules.cend(), [&listOfWords](RulePair rule) 
	{ 
		// std::remove_if(listOfWords.begin(), listOfWords.end(), [&rule](WordsList::value_type metaword) -> bool 
		//{ 
		//	return (Rule_Abstract::Drop == (rule.first)->Run(metaword) ? true : false); 
	//	});
	});
}

};
// namespace WordJewelsEngine
