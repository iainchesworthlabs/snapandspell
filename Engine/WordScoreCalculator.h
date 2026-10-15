#pragma once
#ifndef _WORDSCORE_CALCULATOR_H
#define _WORDSCORE_CALCULATOR_H

#include "WordsList.h"

namespace WordJewelsEngine
{

class WordScoreCalculator
{
public:
	WordScoreCalculator();
	~WordScoreCalculator();

private:
	WordScoreCalculator(const WordScoreCalculator&);
	WordScoreCalculator& operator=(const WordScoreCalculator&);

public:
	void Calculate(WordsList& words);

private:
	void Rule_ScoreByLetterFrequency(WordsList& words);
	void Rule_ThreeLettersOrMore(WordsList& words);
};

}
// namespace WordJewelsEngine

#endif // _WORDSCORE_CALCULATOR_H
