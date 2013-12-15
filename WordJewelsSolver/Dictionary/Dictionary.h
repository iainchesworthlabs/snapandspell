#pragma once
#include <atomic>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "TrieMap.h"

namespace WordJewelsEngine
{

class Dictionary
{
private:
	static const std::wstring DEFAULT_DICTIONARY;

public:
	typedef trie_map<const std::wstring, uint64_t> dictionary_t;
	typedef dictionary_t::const_iterator const_iterator;
	typedef dictionary_t::iterator iterator;
	typedef dictionary_t::key_type key_type;

public:
	Dictionary();
	Dictionary(const std::wstring& dictionary);
	virtual ~Dictionary();

public:
	const_iterator begin() const;
	iterator begin();
	const_iterator end() const;
	iterator end();

public:
	const_iterator FindMatch(const std::wstring& needle) const;
	iterator FindMatch(const std::wstring& needle);
	bool IsCompleteWord(const_iterator& cit) const;
	bool IsCompleteWord(iterator& it) const;

public:
	void LoadDefaultDictionary();
	void LoadDictionary(const std::wstring& dictionary);
	void LoadDictionaryAsync(const std::wstring& dictionary);
	bool IsDictionaryLoaded() const;
	void WaitForDictionaryToLoad() const;

private:
	std::promise<bool> m_DictionaryPromise;
	std::future<bool> m_DictionaryIsLoaded;
	std::thread m_DictionaryLoader;

private:
	std::wstring m_DictionaryPath;
	dictionary_t m_Dictionary;
	uint64_t m_LoadedWords;
};

};
// namespace WordJewelsEngine
