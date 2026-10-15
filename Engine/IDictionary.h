#pragma once
#ifndef _IDICTIONARY_H
#define _IDICTIONARY_H

#include <chrono>
#include <cstdint>
#include <exception>
#include <fstream>
#include <future>
#include <string>
#include <thread>

#include "ActionTimer.h"
#include "CultureInfo.h"
#include "DebugStream.h"
#include "TracedThrow.h"

namespace WordJewelsEngine
{

template<typename DICTIONARY>
class IDictionary
{
public:
	IDictionary() :
		m_DictionaryPromise(),
		m_DictionaryIsLoaded(),
		m_DictionaryLoader(),
		m_DictionaryPath(L""),
		m_Dictionary(),
		m_LoadedWords(0)
	{
		LoadDictionary_Async(DEFAULT_DICTIONARY);
	}

	IDictionary(const std::wstring& dictionary) :
		m_DictionaryPromise(),
		m_DictionaryIsLoaded(),
		m_DictionaryLoader(),
		m_DictionaryPath(L""),
		m_Dictionary(),
		m_LoadedWords(0)
	{
		if (0 >= dictionary.length())
		{
			wdebugstream() << TEXT("DEBUG: Invalid filename or path provided for dictionary: ") << dictionary << TEXT("!") << std::endl;
			DiagnosticsTracedThrow(std::runtime_error("DEBUG: Invalid dictionary specified!"));
		}
		else
		{
			WIN32_FIND_DATA FindFileData;
			HANDLE hDictionary(::FindFirstFile(dictionary.c_str(), &FindFileData));

			if (INVALID_HANDLE_VALUE == hDictionary)
			{
				wdebugstream() << TEXT("DEBUG: Failed to find specified dictionary...not loading dictionary!") << std::endl;
				DiagnosticsTracedThrow(std::runtime_error("DEBUG: Failed to load dictionary!"));
			}
			else
			{
				::FindClose(hDictionary);
				LoadDictionary_Async(dictionary);
			}
		}
	}

	virtual ~IDictionary()
	{
		wdebugstream() << TEXT("DEBUG: Destroying object...waiting for dictionary load") << std::endl;
		WaitForDictionaryToLoad();
		wdebugstream() << TEXT("DEBUG: Destroying object...wait complete") << std::endl;
	}

public:
	virtual typename DICTIONARY::const_iterator begin() const = 0;
	virtual typename DICTIONARY::iterator begin() = 0;
	virtual typename DICTIONARY::const_iterator end() const = 0;
	virtual typename DICTIONARY::iterator end() = 0;

public:
	virtual typename DICTIONARY::const_iterator FindMatch(const std::wstring& needle) const = 0;
	virtual typename DICTIONARY::iterator FindMatch(const std::wstring& needle) = 0;

protected:
	virtual bool InsertWord(const std::wstring& word) = 0;

protected:
	bool IsDictionaryLoaded() const
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

	void WaitForDictionaryToLoad() const
	{
		if (!(m_DictionaryIsLoaded.valid()))
		{
			// Future is not valid...do not continue.
			wdebugstream() << TEXT("DEBUG: Future is not valid...cannot determine if dictionary is loaded") << std::endl;
		}
		else
		{
			// Wait for the dictionary to have loaded.
			wdebugstream() << TEXT("DEBUG: Blocking wait for dictionary to have loaded") << std::endl;

			Diagnostics::ActionTimer actionToTime;
			actionToTime.Start();
			m_DictionaryIsLoaded.wait();
			actionToTime.Stop();

			wdebugstream() << TEXT("DEBUG: Blocking wait for dictionary load finished; duration = ") << actionToTime.ElapsedInMs().count() << TEXT("ms") << std::endl;
		}
	}

private:
	bool LoadDefaultDictionary()
	{
		LoadDictionary(DEFAULT_DICTIONARY);
	}

	bool LoadDictionary(const std::wstring& dictionary)
	{
		std::wifstream stream(dictionary, std::wifstream::in);
		bool bDictionaryIsLoaded(false);

		if (!stream.is_open())
		{
			DiagnosticsTracedThrow(std::runtime_error("SearchForWords::LoadAndParseDictionary - cannot open dictionary file"));
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

			for (/* do nothing */; it != itend; ++it)
			{
				std::wstring newWord(*it);
				if (!InsertWord(*it))
				{
					// Error case
				}
				else
				{
					// Good case
				}
			}

			m_DictionaryPath = dictionary;
			bDictionaryIsLoaded = true;

			// Debug information.
			wdebugstream() << TEXT("DEBUG: Dictionary loaded, total word count: ") << m_LoadedWords << std::endl;
		}

		return bDictionaryIsLoaded;
	}

	void LoadDictionary_Async(const std::wstring& dictionary)
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

protected:
	static const std::wstring DEFAULT_DICTIONARY;
	DICTIONARY m_Dictionary;
	uint64_t m_LoadedWords;

private:
	std::promise<bool> m_DictionaryPromise;
	std::future<bool> m_DictionaryIsLoaded;
	std::thread m_DictionaryLoader;

private:
	std::wstring m_DictionaryPath;
};

template<typename DICTIONARY> const std::wstring IDictionary<DICTIONARY>::DEFAULT_DICTIONARY =
	std::wstring(TEXT("Dictionary/dictionary.")) + CultureInfo().GetISO639_ShortCode(CultureInfo().CurrentLanguage()) + std::wstring(TEXT(".txt"));

}
// namespace WordJewelsEngine

#endif // _IDICTIONARY_H
