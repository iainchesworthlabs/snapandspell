#pragma once
#ifndef _WORDJEWELSENGINE_H
#define _WORDJEWELSENGINE_H

#include <mutex>
#include <string>

#include "Dictionary.h"
#include "GameBoard.h"
#include "GameBoardNode.h"
#include "RulesEngine.h"
#include "Rule_Cloner.h"
#include "Rule_IsUniqueWord.h"
#include "Rule_ThreeLettersOrMore.h"
#include "SearchForWords.h"
#include "WordsList.h"

namespace WordJewelsEngine
{
	
class Engine
{
public:
	Engine();
	~Engine();

private:
	Engine(const Engine& rhs);
	Engine& operator=(const Engine& rhs);

public:
	class Sorter
	{
	public:
		bool operator()(const WordsList::value_type& lhs, const WordsList::value_type& rhs)
		{
			const std::wstring& lhsStr(std::get<0>(lhs));
			const std::wstring& rhsStr(std::get<0>(rhs));

			return (lhsStr.length() < rhsStr.length());
		}
	}
	MetaWordSorter;

public:
	Dictionary m_Dictionary;
	SearchForWords m_Searcher;
	GameBoard m_Gameboard;
	WordsList m_DiscoveredWords;
	RulesEngine m_Rules;

private:
	std::mutex m_EngineLock;
};	

}
// namespace WordJewelsEngine

#endif // _WORDJEWELSENGINE_H
