#pragma once
#include <cstdint>
#include <list>
#include <string>
#include <tuple>
#include <vector>
#include "Dictionary.h"
#include "GameBoard.h"
#include "WordsList.h"

namespace WordJewelsEngine
{

class SearchForWords
{
private:
	typedef std::pair<int32_t, int32_t> neighbouroffset_t;
	typedef std::vector<neighbouroffset_t> neighbours_t;
	typedef std::tuple<uint32_t, uint32_t, std::wstring> partialword_t;
	typedef std::vector<partialword_t> partialpossibilities_t;
	typedef std::list<partialpossibilities_t> partialwordlist_t;

public:
	SearchForWords();
	SearchForWords(const SearchForWords& rhs);
	virtual ~SearchForWords();

public:
	SearchForWords& operator=(const SearchForWords& rhs);
	bool operator==(const SearchForWords& rhs) const;

public:
	void FindWords(const GameBoard& gameBoard, const Dictionary& dictionary, WordsList& discoveredWords);
	
private:
	bool IsValidNeighbour(const neighbouroffset_t& current, const neighbouroffset_t& neighbour) const;
	bool LetterNeverPreviouslyVisited(const partialpossibilities_t& previousLetters, const neighbouroffset_t& neighbour) const;
	bool ExistsInDictionary(const Dictionary& dictionary, const partialword_t& partial, const std::wstring& letter, Dictionary::const_iterator& location) const;

private:
	neighbours_t m_NearestNeighbours;
};

};
// namespace WordJewelsEngine
