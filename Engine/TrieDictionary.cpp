#include "pch.h"
#include "ActionTimer.h"
#include "DebugStream.h"
#include "TracedThrow.h"
#include "TrieDictionary.h"

namespace WordJewelsEngine
{

TrieDictionary::TrieDictionary() : 
	IDictionary<Dictionary_TrieType>()
{
}

TrieDictionary::~TrieDictionary()
{
}

TrieDictionary::const_iterator TrieDictionary::begin() const
{
	wdebugstream() << TEXT("DEBUG: TrieDictionary::begin() const") << std::endl;
	if (!IsDictionaryLoaded()) WaitForDictionaryToLoad();
	return m_Dictionary.begin();
}

TrieDictionary::iterator TrieDictionary::begin()
{
	wdebugstream() << TEXT("DEBUG: TrieDictionary::begin()") << std::endl;
	if (!IsDictionaryLoaded()) WaitForDictionaryToLoad();
	return m_Dictionary.begin();
}

TrieDictionary::const_iterator TrieDictionary::end() const
{
	wdebugstream() << TEXT("DEBUG: TrieDictionary::end() const") << std::endl;
	if (!IsDictionaryLoaded()) WaitForDictionaryToLoad();
	return m_Dictionary.end();
}

TrieDictionary::iterator TrieDictionary::end()
{
	wdebugstream() << TEXT("DEBUG: TrieDictionary::end()") << std::endl;
	if (!IsDictionaryLoaded()) WaitForDictionaryToLoad();
	return m_Dictionary.end();
}

TrieDictionary::const_iterator TrieDictionary::FindMatch(const std::wstring& needle) const
{
	wdebugstream() << TEXT("DEBUG: TrieDictionary::FindMatch(") << needle << TEXT(") const") << std::endl;
	if (!IsDictionaryLoaded()) WaitForDictionaryToLoad();
	return m_Dictionary.find(TrieDictionary::key_type(needle));
}

TrieDictionary::iterator TrieDictionary::FindMatch(const std::wstring& needle)
{
	wdebugstream() << TEXT("DEBUG: TrieDictionary::FindMatch(") << needle << TEXT(")") << std::endl;
	if (!IsDictionaryLoaded()) WaitForDictionaryToLoad();
	return m_Dictionary.find(TrieDictionary::key_type(needle));
}

bool TrieDictionary::IsCompleteWord(TrieDictionary::const_iterator& cit) const
{
	return (cit.is_word());
}

bool TrieDictionary::IsCompleteWord(TrieDictionary::iterator& it) const
{
	return (it.is_word());
}

bool TrieDictionary::InsertWord(const std::wstring& word)
{
	std::wstring lowerWord(word);

	std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), ::tolower);
	m_Dictionary.insert(std::make_pair(TrieDictionary::key_type(lowerWord), ++m_LoadedWords));
	
	return true;
}

};
// namespace WordJewelsEngine
