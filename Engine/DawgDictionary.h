#pragma once
#ifndef _DAWGDICTIONARY_H
#define _DAWGDICTIONARY_H

#include <iterator>
#include <string>

#include "IDictionary.h"

namespace WordJewelsEngine
{
	//
	// http://pages.pathcom.com/~vadco/dawg.html!!!
	//

class DawgIterator
{
public:
	typedef uint32_t const_iterator;
	typedef uint32_t iterator;
	typedef uint32_t key_type;
	
public:
	void clear();
};

class DawgDictionary : public IDictionary<DawgIterator>
{
public:
	typedef DawgIterator::const_iterator const_iterator;
	typedef DawgIterator::iterator iterator;
	typedef DawgIterator::key_type key_type;

public:
	DawgDictionary();
	~DawgDictionary();

public:
	virtual const_iterator begin() const;
	virtual iterator begin();
	virtual const_iterator end() const;
	virtual iterator end();

public:
	virtual const_iterator FindMatch(const std::wstring& needle) const;
	virtual iterator FindMatch(const std::wstring& needle);

private:
	virtual bool InsertWord(const std::wstring& word);

private:

};

}
// namespace WordJewelsEngine

#endif // _DAWGDICTIONARY_H
