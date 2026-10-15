#pragma once
#ifndef _TRIEDICTIONARY_H
#define _TRIEDICTIONARY_H

#include <cstdint>
#include <string>

#include "IDictionary.h"
#include "TrieMap.h"

namespace WordJewelsEngine
{

typedef trie_map<const std::wstring, uint64_t> Dictionary_TrieType;

class TrieDictionary : public IDictionary<Dictionary_TrieType>
{
public:
	typedef Dictionary_TrieType::const_iterator const_iterator;
	typedef Dictionary_TrieType::iterator iterator;
	typedef Dictionary_TrieType::key_type key_type;

public:
	TrieDictionary();
	~TrieDictionary();

public:
	virtual const_iterator begin() const;
	virtual iterator begin();
	virtual const_iterator end() const;
	virtual iterator end();

public:
	virtual const_iterator FindMatch(const std::wstring& needle) const;
	virtual iterator FindMatch(const std::wstring& needle);

public:
	bool IsCompleteWord(const_iterator& cit) const;
	bool IsCompleteWord(iterator& it) const;

private:
	virtual bool InsertWord(const std::wstring& word);
};

};
// namespace WordJewelsEngine

#endif // _TRIEDICTIONARY_H
