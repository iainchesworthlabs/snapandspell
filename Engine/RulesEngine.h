#pragma once
#include <cstdint>
#include <vector>
#include "Rule_Cloner.h"
#include "WordsList.h"

namespace WordJewelsEngine
{

class RulesEngine
{
private:
	typedef std::pair<Rule_Cloner, int64_t> RulePair;

public:
	RulesEngine();
	virtual ~RulesEngine();

public:
	void AddRule(Rule_Cloner rule, int64_t priority);
	void Execute(WordsList& listOfWords) const;

private:
	class RuleComparer
	{
	public:
		bool operator()(const RulePair& lhs, const RulePair& rhs)
		{
			return (lhs.second < rhs.second);
		}
	};
	
private:
	std::vector<RulePair> m_Rules;
};

};
// namespace WordJewelsEngine
