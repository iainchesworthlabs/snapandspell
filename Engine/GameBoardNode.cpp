#include "pch.h"
#include "GameBoardNode.h"
#include <algorithm>
#include <locale>

namespace WordJewelsEngine
{

const std::wstring GameBoardNode::EMPTY_NODE(L"");
const std::wstring GameBoardNode::LETTER_QU(L"qu");
const std::wstring GameBoardNode::LETTER_Q(L"q");

GameBoardNode::GameBoardNode(const std::wstring& letter) : m_Letter(letter)
{
	// Uses the "classic" (C) locale, but this works with any.
	std::locale loc(std::locale::classic());

	// Search mask specifying characters to search for...
	std::ctype_base::mask m(std::ctype_base::alpha);

	// Make sure the characters are all lowercase
	std::for_each(m_Letter.begin(), m_Letter.end(), [&loc](std::wstring::value_type character) 
	{ 
		character = std::tolower(character, loc); 
	});

	// Make sure that each character is alphabetic
	std::for_each(m_Letter.begin(), m_Letter.end(), [this, &loc](std::wstring::value_type character) 
	{ 
		if (!std::isalpha(character, loc))
		{
			throw std::invalid_argument("GameBoardBuffer::load - invalid character");
		}
		else if (LETTER_Q == m_Letter)
		{
			// Character's okay...but add the following 'u'
			m_Letter = LETTER_QU;
		}
		else
		{
			// Character's okay!
		}
	});

	if ((1 < m_Letter.length()) && (LETTER_QU != m_Letter))
	{
		throw std::invalid_argument("GameBoardBuffer::load - invalid character");
	}
	else
	{
		// Standard alpha character...do nothing.
	}
}

GameBoardNode::GameBoardNode(const GameBoardNode& rhs) : m_Letter(rhs.m_Letter)
{
}

GameBoardNode::~GameBoardNode()
{
}

GameBoardNode& GameBoardNode::operator=(const GameBoardNode& rhs)
{
	if (&rhs != this)
	{
		// Do any required copying...
		m_Letter = rhs.m_Letter;
	}

	return *this;
}

bool GameBoardNode::operator==(const GameBoardNode& rhs)
{
	typedef std::wstring::const_iterator cit;
	typedef std::pair<cit, cit> mismatchposition_t;

	mismatchposition_t mmPos = std::mismatch(m_Letter.cbegin(), m_Letter.cend(), rhs.m_Letter.cbegin());
		
	return ((m_Letter.cend() == mmPos.first) && (rhs.m_Letter.cend() == mmPos.second));
}

const std::wstring GameBoardNode::Letter() const
{
	return m_Letter;
}

const std::wstring GameBoardNode::operator()()
{
	return m_Letter;
}

};
// namespace WordJewelsEngine
