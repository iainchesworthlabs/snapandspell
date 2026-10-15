#pragma once
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace WordJewelsEngine
{

class WordsList
{
private:
	typedef std::tuple<std::wstring, int64_t> metaword_t;
	typedef std::vector<metaword_t> metawordlist_t;

public:
	WordsList();
	WordsList(const WordsList& rhs);
	virtual ~WordsList();

public:
	typedef metawordlist_t::reference	   reference;
	typedef metawordlist_t::value_type	   value_type;
	typedef metawordlist_t::const_iterator const_iterator;
	typedef metawordlist_t::iterator	   iterator;

public:
	WordsList& operator=(const WordsList& rhs);

public:
	const_iterator begin() const;
	iterator begin();
	const_iterator end() const;
	iterator end();

public:
	void add(std::wstring& word, int64_t score = 0);
	void clear();
	int64_t count() const;

private:
	metawordlist_t m_WordsList;
};

};
// namespace WordJewelsEngine
