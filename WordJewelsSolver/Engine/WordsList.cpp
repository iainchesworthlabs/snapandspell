#include "Stdafx.h"
#include "WordsList.h"

namespace WordJewelsEngine
{

WordsList::WordsList() : m_WordsList()
{
}

WordsList::~WordsList()
{
}

WordsList::const_iterator WordsList::begin() const
{
	return m_WordsList.cbegin();
}

WordsList::iterator WordsList::begin()
{
	return m_WordsList.begin();
}

WordsList::const_iterator WordsList::end() const
{
	return m_WordsList.cend();
}

WordsList::iterator WordsList::end()
{
	return m_WordsList.end();
}

void WordsList::add(std::wstring& word, int64_t score)
{
	metaword_t metaWord(std::make_tuple(word, score));
	m_WordsList.push_back(metaWord);
}

void WordsList::clear()
{
	m_WordsList.clear();
}

int64_t WordsList::count() const
{
	return m_WordsList.size();
}

};
// namespace WordJewelsEngine
