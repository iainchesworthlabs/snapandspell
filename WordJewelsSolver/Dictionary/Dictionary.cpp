#include "Stdafx.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>

#include "Dictionary.h"
#include "DebugStream.h"

namespace WordJewelsEngine
{

const std::wstring Dictionary::DEFAULT_DICTIONARY = L"Dictionary/dictionary.en.txt";

Dictionary::Dictionary() : 
	m_DictionaryPromise(),
	m_DictionaryIsLoaded(),
	m_DictionaryLoader(),
	m_DictionaryPath(L""), 
	m_Dictionary(), 
	m_LoadedWords(0)
{	
	LoadDictionaryAsync(DEFAULT_DICTIONARY);
}

Dictionary::Dictionary(const std::wstring& dictionary) :
	m_DictionaryPromise(),
	m_DictionaryIsLoaded(),
	m_DictionaryLoader(), 
	m_DictionaryPath(L""), 
	m_Dictionary(), 
	m_LoadedWords(0)
{
	LoadDictionaryAsync(dictionary);
}

Dictionary::~Dictionary()
{
	wdebugstream() << TEXT("DEBUG: Destroying object...waiting for dictionary load") << std::endl;
	WaitForDictionaryToLoad();
	wdebugstream() << TEXT("DEBUG: Destroying object...wait complete") << std::endl;
}

void Dictionary::LoadDictionaryAsync(const std::wstring& dictionary)
{
	m_DictionaryLoader = std::thread([&]()
	{
		try
		{
			std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> start_time, end_time;
			std::thread::id loaderThreadId(std::this_thread::get_id());

			wdebugstream() << TEXT("DEBUG: Loading dictionary in thread id: ") << TEXT("0x") << std::hex << loaderThreadId << std::endl;

			start_time = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
			LoadDictionary(dictionary);
			end_time = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());

			wdebugstream() << TEXT("DEBUG: Finished loading dictionary in thread id: ") << TEXT("0x") << std::hex << loaderThreadId;
			wdebugstream() << TEXT(", duration = ") << std::dec << (end_time - start_time).count() << TEXT("ms") << std::endl;
		}
		catch (...)
		{
			wdebugstream() << TEXT("DEBUG: Failure during asynchronous dictionary loading") << std::endl;
		}

		wdebugstream() << TEXT("DEBUG: Finished trying to load...indicate completion") << std::endl;
		m_DictionaryPromise.set_value(true);
	});

	m_DictionaryLoader.detach();
	m_DictionaryIsLoaded = m_DictionaryPromise.get_future();
}

bool Dictionary::IsDictionaryLoaded() const
{
	bool dictLoaded(false);

	if (!(m_DictionaryIsLoaded.valid()))
	{
		// Future is not valid...do not continue.
		wdebugstream() << TEXT("DEBUG: Future is not valid...cannot determine if dictionary is loaded") << std::endl;
	}
	else if (std::future_status::ready != m_DictionaryIsLoaded.wait_for(std::chrono::seconds(0)))
	{
		// Future is not yet ready...do not continue.
		wdebugstream() << TEXT("DEBUG: Dictionary is currently still loading") << std::endl;
	}
	else
	{
		// Yup, everything is good.
		wdebugstream() << TEXT("DEBUG: Dictionary indicates it has loaded") << std::endl;
		dictLoaded = true;
	}

	return dictLoaded;
}

void Dictionary::WaitForDictionaryToLoad() const
{	
	if (!(m_DictionaryIsLoaded.valid()))
	{
		// Future is not valid...do not continue.
		wdebugstream() << TEXT("DEBUG: Future is not valid...cannot determine if dictionary is loaded") << std::endl;
	}
	else
	{
		std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> start_time, end_time;

		// Wait for the dictionary to have loaded.
		wdebugstream() << TEXT("DEBUG: Blocking wait for dictionary to have loaded") << std::endl;

		start_time = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
		m_DictionaryIsLoaded.wait();
		end_time = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());

		wdebugstream() << TEXT("DEBUG: Blocking wait for dictionary load finished; duration = ") << (end_time - start_time).count() << TEXT("ms") << std::endl;
	}
}

Dictionary::const_iterator Dictionary::begin() const
{
	if (!IsDictionaryLoaded()) WaitForDictionaryToLoad();
	return m_Dictionary.begin();
}

Dictionary::iterator Dictionary::begin()
{
	if (!IsDictionaryLoaded()) WaitForDictionaryToLoad();
	return m_Dictionary.begin();
}

Dictionary::const_iterator Dictionary::end() const
{
	if (!IsDictionaryLoaded()) WaitForDictionaryToLoad();
	return m_Dictionary.end();
}

Dictionary::iterator Dictionary::end()
{
	if (!IsDictionaryLoaded()) WaitForDictionaryToLoad();
	return m_Dictionary.end();
}

Dictionary::const_iterator Dictionary::FindMatch(const std::wstring& needle) const
{
	if (!IsDictionaryLoaded()) WaitForDictionaryToLoad();
	return m_Dictionary.find(Dictionary::key_type(needle));
}

Dictionary::iterator Dictionary::FindMatch(const std::wstring& needle)
{
	if (!IsDictionaryLoaded()) WaitForDictionaryToLoad();
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
	}

	// Debug information.
	wdebugstream() << TEXT("DEBUG: Dictionary loaded, total word count: ") << m_LoadedWords << std::endl;
}

};
// namespace WordJewelsEngine
