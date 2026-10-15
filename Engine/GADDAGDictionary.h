#pragma once
#ifndef _GADDAGDICTIONARY_H
#define _GADDAGDICTIONARY_H

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "IDictionary.h"

namespace WordJewelsEngine
{

class GADDAGDictionary;

class TrieNode
{
	friend class GADDAGDictionary;

public:
	TrieNode() 
	{ 
	}

	TrieNode(char letter, bool isTerminal, int firstChildOffset, bool isLastSibling = false) : 
		m_Letter(letter),
		m_FirstChildOffset(firstChildOffset),
		m_IsTerminal(isTerminal),
		m_IsLastSibling(isLastSibling)
	{ 
	}

	char letter() const 
	{ 
		return m_Letter; 
	}

	bool isTerminal() const 
	{ 
		return m_IsTerminal; 
	}

	const TrieNode* firstChild() const 
	{
		if (0 == m_FirstChildOffset)
		{
			return nullptr;
		}
		else
		{
			return this + m_FirstChildOffset;
		}
	}

	// note this depends on all siblings being directly adjacent in memory
	const TrieNode* nextSibling() const 
	{
		if (isLastSibling())
		{
			return nullptr;
		}
		else
		{
			return this + 1;
		}
	}

	const TrieNode* child(unsigned char c) const 
	{
		for (const TrieNode* child = firstChild(); child != nullptr; child = child->nextSibling()) 
		{
			if (child->letter() == c)
			{
				return child;
			}
			else if (child->letter() > c)
			{
				return nullptr;
			}
		}

		return nullptr;
	}

	bool operator==(TrieNode other) const 
	{ 
		return asInt() == other.asInt(); 
	}

	bool operator!=(TrieNode other) const 
	{ 
		return asInt() != other.asInt(); 
	}

	bool operator< (TrieNode other) const 
	{ 
		return asInt() < other.asInt(); 
	}

	int asInt() const 
	{ 
		return *((int32_t*) this); 
	}

private:
	bool isLastSibling() const 
	{ 
		return m_IsLastSibling; 
	}

	uint8_t m_Letter : 8;
	int32_t m_FirstChildOffset : 22;
	bool m_IsTerminal : 1;
	bool m_IsLastSibling : 1;
};

class Trie 
{
public:
	Trie(const std::wstring& file);
	~Trie();

	const TrieNode* root() const 
	{
		return &nodes[0]; 
	}

private:
	const TrieNode* nodes;
	size_t mapLength;
};

class GADDAGDictionaryIterator
{
public:
	typedef uint32_t const_iterator;
	typedef uint32_t iterator;
	typedef uint32_t key_type;

public:
	void clear();
};

class GADDAGDictionary : public IDictionary<GADDAGDictionaryIterator>
{
public:
	typedef GADDAGDictionaryIterator::const_iterator const_iterator;
	typedef GADDAGDictionaryIterator::iterator iterator;
	typedef GADDAGDictionaryIterator::key_type key_type;

public:
	GADDAGDictionary() :
		IDictionary<GADDAGDictionaryIterator>(),
		m_Done(false) 
	{
		m_NumReservedNodes = 65536;
		m_Nodes = (TrieNode*) calloc (m_NumReservedNodes, sizeof(TrieNode));

		if (m_Nodes == nullptr)
		{
			perror("Can't allocate TrieNode memory");
			abort();
		}

		m_NumNodes = 0;
		m_Nodes[m_NumNodes++] = TrieNode('*', false, 1, true);

		//ReadNextWord(); ///FIXME

		m_Nodes[0].m_FirstChildOffset = BuildNode(0);

		// relativate child indices
		for (int i = 0; i < m_NumNodes; ++i)
		{
			if (m_Nodes[i].m_FirstChildOffset != 0)
			{
				m_Nodes[i].m_FirstChildOffset -= i;
			}
		}
	}

	~GADDAGDictionary()
	{
		free(m_Nodes);
	}

private:
	struct NodeVectorHash 
	{
		size_t operator()(const std::vector<TrieNode>& nodes) const 
		{
			size_t result = 0;

			// FIXME is this a good hash on 32-bit architectures?

			for (uint32_t i = 0; i < nodes.size(); ++i)
			{
				result += nodes[i].asInt();
			}

			return result;
		}
	};

	typedef std::unordered_map<std::vector<TrieNode>, int32_t, NodeVectorHash> NodeCache;

private:
	int32_t BuildNode(uint32_t depth)
	{
		// returns index of first child
		assert(!m_Done);

		if (m_CurrentWord.length() == depth) 
		{
			// End of word reached. If the next word isn't a continuation of
			// the current one, then we've reached the bottom of the recursion
			// tree.

			//ReadNextWord();  ///FIXME

			if (m_FirstDiff < depth)
			{
				return 0;
			}
		}

		std::vector<TrieNode> edges;	// FIXME find good name

		// Loop through all the sub-nodes until a word is read which can't be
		// reached via this node.

		do 
		{
			wchar_t letter = m_CurrentWord[depth];
			bool isTerminal = ((m_CurrentWord.length() - 1) == depth);
			edges.push_back(TrieNode(letter, isTerminal, BuildNode(depth + 1)));
		}  
		while (m_FirstDiff == depth && !m_Done);

		if (!m_Done)
		{
			assert(m_FirstDiff < depth);
		}

		edges.back().m_IsLastSibling = true;

		return AddNodes(edges);
	}

	int32_t AddNodes(const std::vector<TrieNode>& edges)
	{
		NodeCache::const_iterator n = m_NodeCache.find(edges);
		if (n != m_NodeCache.end())
		{
			return n->second;
		}

		while ((m_NumNodes + edges.size()) > m_NumReservedNodes) 
		{
			m_NumReservedNodes *= 2;
			m_Nodes = (TrieNode*)realloc(m_Nodes, m_NumReservedNodes * sizeof(TrieNode));
		}

		int32_t firstFreeIndex = m_NumNodes;

		for (uint32_t i = 0; i < edges.size(); ++i)
		{
			m_Nodes[m_NumNodes + i] = edges[i];
		}

		m_NumNodes += edges.size();
		m_NodeCache[edges] = firstFreeIndex;

		return firstFreeIndex;
	}

public:
	virtual const_iterator begin() const;
	virtual iterator begin();
	virtual const_iterator end() const;
	virtual iterator end();

public:
	virtual const_iterator FindMatch(const std::wstring& needle) const;
	virtual iterator FindMatch(const std::wstring& needle);

private:
	virtual bool InsertWord(const std::wstring& word)
	{
		int numCommonLetters = 0;
		int l = std::min(m_CurrentWord.length(), word.length());

		while ((m_CurrentWord[numCommonLetters] == word[numCommonLetters]) && (numCommonLetters < l))
		{
			++numCommonLetters;
		}

		m_FirstDiff = numCommonLetters;
		m_CurrentWord = word;
	}

private:
	std::wstring m_CurrentWord;
	uint32_t m_FirstDiff;	// index of first letter where _currentWord differs from the last word
	int32_t m_NumNodes;
	int32_t m_NumReservedNodes;
	bool m_Done;

	NodeCache m_NodeCache;
	TrieNode* m_Nodes;
};

}
// namespace WordJewelsEngine

#endif // _GADDAGDICTIONARY_H
