#include "pch.h"

#include <algorithm>

#include "ActionTimer.h"
#include "DebugStream.h"
#include "Rule_ScoreByLetterFrequency.h"
#include "Rule_ThreeLettersOrMore.h"
#include "WordScoreCalculator.h"

namespace WordJewelsEngine
{

WordScoreCalculator::WordScoreCalculator()
{
}

WordScoreCalculator::~WordScoreCalculator()
{
}

void WordScoreCalculator::Calculate(WordsList& words)
{
	Diagnostics::ActionTimer actionToTime;
	actionToTime.Start();
	Rule_ScoreByLetterFrequency(words);
	actionToTime.Stop();
	wdebugstream() << TEXT("DEBUG: WordJewelsEngine::Rule_ScoreByLetterFrequency - duration: ") << actionToTime.ElapsedInMs().count() << TEXT("ms") << std::endl;

	actionToTime.Reset();
	actionToTime.Start();
	Rule_ThreeLettersOrMore(words);
	actionToTime.Stop();
	wdebugstream() << TEXT("DEBUG: WordJewelsEngine::Rule_ThreeLettersOrMore - duration: ") << actionToTime.ElapsedInMs().count() << TEXT("ms") << std::endl;
}

void WordScoreCalculator::Rule_ScoreByLetterFrequency(WordsList& words)
{
	wdebugstream() << TEXT("DEBUG: WordJewelsEngine::Rule_ScoreByLetterFrequency") << std::endl;

	std::for_each(words.begin(), words.end(), [](WordsList::reference element)
	{
		const std::wstring& wordToScore(std::get<0>(element));
		const uint64_t returnedScore(std::get<1>(element) = WordJewelsEngine::Rule_ScoreByLetterFrequency(wordToScore.begin(), wordToScore.end()));

		wdebugstream() << TEXT("   ") << TEXT(" - ");
		wdebugstream() << wordToScore << TEXT("/") << returnedScore;
		wdebugstream() << std::endl;
	});
}

void WordScoreCalculator::Rule_ThreeLettersOrMore(WordsList& words)
{
	wdebugstream() << TEXT("DEBUG: NOT IMPLEMENTED - WordJewelsEngine::Rule_ThreeLettersOrMore") << std::endl;
}

}
// namespace WordJewelsEngine
