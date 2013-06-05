#include "Stdafx.h"
#include <algorithm>
#include <exception>
#include <functional>
#include <iostream>
#include "SearchForWords.h"

namespace WordJewelsEngine
{

SearchForWords::SearchForWords() : m_NearestNeighbours()
{
	m_NearestNeighbours.push_back(std::make_pair( 1,  0));
	m_NearestNeighbours.push_back(std::make_pair( 1, -1));
	m_NearestNeighbours.push_back(std::make_pair( 0, -1));
	m_NearestNeighbours.push_back(std::make_pair(-1, -1));
	m_NearestNeighbours.push_back(std::make_pair(-1,  0));
	m_NearestNeighbours.push_back(std::make_pair(-1,  1));
	m_NearestNeighbours.push_back(std::make_pair( 0,  1));
	m_NearestNeighbours.push_back(std::make_pair( 1,  1));
}

SearchForWords::SearchForWords(const SearchForWords& rhs)
{
}

SearchForWords::~SearchForWords()
{
}

SearchForWords& SearchForWords::operator=(const SearchForWords& rhs)
{
	if (this == &rhs)
	{
		// Prevent self-assignment
	}
	else
	{
		// Do any copying here
	}

	return *this;
}

bool SearchForWords::operator==(const SearchForWords& rhs) const
{
	return true;
}

void SearchForWords::FindWords(const GameBoard& gameBoard, const Dictionary& dictionary, WordsList& discoveredWords)
{
	partialwordlist_t listOfPartialWords;

	// Make sure any previously found words are removed.
	discoveredWords.clear();

	for(GameBoard::iterator gbit = gameBoard.begin(); gbit != gameBoard.end(); ++gbit)
	{
		partialpossibilities_t possiblePartialSeed;
			
		possiblePartialSeed.push_back(std::make_tuple(gbit.Row(), gbit.Column(), (*gbit)()));
		listOfPartialWords.push_back(possiblePartialSeed);

		while(!listOfPartialWords.empty())
		{
			// Extract the top of the list of partial words to think about.
			partialpossibilities_t currentPartial(listOfPartialWords.back());

			for (neighbours_t::iterator nnit = m_NearestNeighbours.begin(); nnit != m_NearestNeighbours.end(); ++nnit)
			{
				const neighbouroffset_t current(std::make_pair(std::get<0>(currentPartial.back()), std::get<1>(currentPartial.back())));
				const neighbouroffset_t neighbour(std::make_pair(current.first + (*nnit).first, current.second + (*nnit).second));
				WordJewelsEngine::Dictionary::const_iterator DictionaryIter;
				
				if (!IsValidNeighbour(current, neighbour))
				{
					// Do nothing because this neighbour does not exist on the board.
				}
				else if (!LetterNeverPreviouslyVisited(currentPartial, neighbour))
				{
					// This letter has been previously visited, don't repeat it.
				}
				else if (!ExistsInDictionary(dictionary, currentPartial.back(), gameBoard.at(neighbour.first, neighbour.second).Letter(), DictionaryIter))
				{
					// Didn't find it in the dictionary...do nothing.
				}
				else
				{
					partialpossibilities_t newPartial(currentPartial);

					// Save this new word as a possible partial for the next round of searching..
					std::wstring possibleWord(std::get<2>(currentPartial.back()) + gameBoard.at(neighbour.first, neighbour.second).Letter());
					newPartial.push_back(std::make_tuple(neighbour.first, neighbour.second, possibleWord));
					listOfPartialWords.push_back(newPartial);

					if (!dictionary.IsCompleteWord(DictionaryIter))
					{
						// Only a partial...don't do anything else.
					}
					else
					{
						// Found a valid word, store it and continue.
						discoveredWords.add(possibleWord);
					}
				}
			}
			
			// Remove the *finish* current partial word...there's nothing else to do.
			listOfPartialWords.erase(std::find(listOfPartialWords.cbegin(), listOfPartialWords.cend(), currentPartial));
		}
	}
}

bool SearchForWords::IsValidNeighbour(const neighbouroffset_t& current, const neighbouroffset_t& neighbour) const
{
	bool validNeighbour(true);

	validNeighbour &= ((0 <= neighbour.first) && (GameBoard::NUMBER_OF_ROWS_PER_BOARD > neighbour.first));
	validNeighbour &= ((0 <= neighbour.second) && (GameBoard::NUMBER_OF_COLUMNS_PER_BOARD > neighbour.second));

	return validNeighbour;
}

bool SearchForWords::LetterNeverPreviouslyVisited(const partialpossibilities_t& previousLetters, const neighbouroffset_t& neighbour) const
{
	partialpossibilities_t::const_iterator ppit;
	
	ppit = std::find_if(previousLetters.cbegin(), previousLetters.cend(), [neighbour](partialword_t partial) -> bool 
	{ 
		bool isMatch(true);

		isMatch &= (std::get<0>(partial) == neighbour.first);
		isMatch &= (std::get<1>(partial) == neighbour.second);

		return isMatch;
	});

	return (previousLetters.cend() == ppit);
}

bool SearchForWords::ExistsInDictionary(const Dictionary& dictionary, const partialword_t& partial, const std::wstring& letter, Dictionary::const_iterator& location) const
{
	const std::wstring tmpLetters(std::get<2>(partial) + letter);
	location = dictionary.FindMatch(tmpLetters);
	return (dictionary.end() != location);
}

};
// namespace WordJewels Engine