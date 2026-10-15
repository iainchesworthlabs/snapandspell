#pragma once
#ifndef _RULE_SCOREBYLETTERFREQUENCY
#define _RULE_SCOREBYLETTERFREQUENCY

#include <cstdint>
#include <utility>
#include <vector>

namespace WordJewelsEngine
{

template<class InputIterator>
uint64_t Rule_ScoreByLetterFrequency(InputIterator first, InputIterator last)
{
	typedef std::pair<wchar_t, double> LetterFrequencyMapping;
	typedef std::vector<LetterFrequencyMapping> LetterFrequencyTable;

	static LetterFrequencyTable lfsTable(
	{
		std::make_pair(L'e', 12.02),
		std::make_pair(L't', 9.10),
		std::make_pair(L'a', 8.12),
		std::make_pair(L'o', 7.68),
		std::make_pair(L'i', 7.31),
		std::make_pair(L'n', 6.95),
		std::make_pair(L's', 6.28),
		std::make_pair(L'r', 6.02),
		std::make_pair(L'h', 5.92),
		std::make_pair(L'd', 4.32),
		std::make_pair(L'l', 3.98),
		std::make_pair(L'u', 2.88),
		std::make_pair(L'c', 2.71),
		std::make_pair(L'm', 2.61),
		std::make_pair(L'f', 2.30),
		std::make_pair(L'y', 2.11),
		std::make_pair(L'w', 2.09),
		std::make_pair(L'g', 2.03),
		std::make_pair(L'p', 1.82),
		std::make_pair(L'b', 1.49),
		std::make_pair(L'v', 1.11),
		std::make_pair(L'k', 0.69),
		std::make_pair(L'x', 0.17),
		std::make_pair(L'q', 0.11),
		std::make_pair(L'j', 0.10),
		std::make_pair(L'z', 0.07)
	});

	uint64_t lettersProcessed(0);
	double letterScore(0.0);

	struct comp
	{
		comp(wchar_t const& c) : _c(c) 
		{ 
		}

		bool operator() (const LetterFrequencyMapping& p)
		{
			return (p.first == _c);
		}

		wchar_t _c;
	};

	for ( /* DO NOTHING */; first != last; ++first)
	{
		LetterFrequencyTable::iterator it(std::find_if(lfsTable.begin(), lfsTable.end(), comp(*first)));
		if (lfsTable.end() != it)
		{
			letterScore += (100.0 / it->second);
		}

		++lettersProcessed;
	}

	return uint64_t(letterScore / lettersProcessed);
}

}
// namespace WordJewelsEngine

#endif // _RULE_SCOREBYLETTERFREQUENCY
