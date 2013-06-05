#include "Stdafx.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include "Dictionary.h"
#include "DebugStream.h"

namespace WordJewelsEngine
{

const std::wstring Dictionary::DEFAULT_DICTIONARY = L"Dictionary/Dictionary_EN.txt";

Dictionary::Dictionary() : 
	m_AsyncDictionary(),
	m_AccessProtection(), 
	m_DictionaryPath(L""), 
	m_Dictionary(), 
	m_DictionaryLoaded(false), 
	m_LoadedWords(0)
{
	InitialiseAsynchronously(DEFAULT_DICTIONARY);
}

Dictionary::Dictionary(const std::wstring& dictionary) : 
	m_AsyncDictionary(),
	m_AccessProtection(), 
	m_DictionaryPath(L""), 
	m_Dictionary(), 
	m_DictionaryLoaded(false), 
	m_LoadedWords(0)
{
	InitialiseAsynchronously(dictionary);
}

Dictionary::~Dictionary()
{
}

void Dictionary::InitialiseAsynchronously(const std::wstring& dictionary)
{
	std::packaged_task<bool()> task([&]()
	{
		try
		{
			LoadDictionary(dictionary);
			m_DictionaryLoaded = true;
		}
		catch (...)
		{
			wdebugstream() << TEXT("DEBUG: Failure during asynchronous dictionary loading") << std::endl;
			m_DictionaryLoaded = false;
		}
		
		return m_DictionaryLoaded;
	}
	);

	m_AsyncDictionary = task.get_future();
	std::thread(std::move(task)).detach();
}

Dictionary::const_iterator Dictionary::begin() const
{
	m_AsyncDictionary.wait();
	std::lock_guard<std::mutex> lock(m_AccessProtection);
	return m_Dictionary.begin();
}

Dictionary::iterator Dictionary::begin()
{
	m_AsyncDictionary.wait();
	std::lock_guard<std::mutex> lock(m_AccessProtection);
	return m_Dictionary.begin();
}

Dictionary::const_iterator Dictionary::end() const
{
	m_AsyncDictionary.wait();
	std::lock_guard<std::mutex> lock(m_AccessProtection);
	return m_Dictionary.end();
}

Dictionary::iterator Dictionary::end()
{
	m_AsyncDictionary.wait();
	std::lock_guard<std::mutex> lock(m_AccessProtection);
	return m_Dictionary.end();
}

Dictionary::const_iterator Dictionary::FindMatch(const std::wstring& needle) const
{
	m_AsyncDictionary.wait();
	std::lock_guard<std::mutex> lock(m_AccessProtection);
	return m_Dictionary.find(Dictionary::key_type(needle));
}

Dictionary::iterator Dictionary::FindMatch(const std::wstring& needle)
{
	m_AsyncDictionary.wait();
	std::lock_guard<std::mutex> lock(m_AccessProtection);
	return m_Dictionary.find(Dictionary::key_type(needle));
}

bool Dictionary::IsCompleteWord(Dictionary::const_iterator& cit) const
{
	return (cit.is_word());
}

bool Dictionary::IsCompleteWord(Dictionary::iterator& it) const
{
	return (it.is_word());
}

void Dictionary::LoadDefaultDictionary()
{
	LoadDictionary(DEFAULT_DICTIONARY);
}

void Dictionary::LoadDictionary(const std::wstring& dictionary)
{
	std::wifstream stream(dictionary, std::wifstream::in);
	if (!stream.is_open())
	{
		throw std::runtime_error("SearchForWords::LoadAndParseDictionary - cannot open dictionary file");
	}
	else
	{
		typedef std::istream_iterator<std::wstring, wchar_t> istream_iter;
		istream_iter it(stream), itend;

		// Debug information.
		wdebugstream() << TEXT("DEBUG: Dictionary about to be loaded: ") << dictionary << std::endl;

		// Prevent concurrent writing of the dictionary.
		std::lock_guard<std::mutex> lock(m_AccessProtection);

		m_DictionaryPath.clear();
		m_Dictionary.clear();
		m_LoadedWords = 0;

		for(/* do nothing */ ; it != itend; ++it)
		{
			std::wstring newWord(*it);
			std::transform(newWord.begin(), newWord.end(), newWord.begin(), ::tolower);

			m_Dictionary.insert(std::make_pair(dictionary_t::key_type(newWord), ++m_LoadedWords));
		}

		m_DictionaryPath = dictionary;
		m_DictionaryLoaded = true;
	}

	// Debug information.
	wdebugstream() << TEXT("DEBUG: Dictionary loaded, total word count: ") << m_LoadedWords << std::endl;
}

bool Dictionary::IsDictionaryLoaded() const
{
	return m_DictionaryLoaded;
}

};
// namespace WordJewelsEngine
