#pragma once
#include <string>

namespace WordJewelsEngine
{

class GameBoardNode
{
public:
	static const std::wstring EMPTY_NODE; 

public:
	GameBoardNode(const std::wstring& letter = L"");
	GameBoardNode(const GameBoardNode& rhs);
	virtual ~GameBoardNode();

public:
	GameBoardNode& operator=(const GameBoardNode& rhs);
	bool operator==(const GameBoardNode& rhs);

public:
	const std::wstring Letter() const;
	const std::wstring operator()();

private:
	static const std::wstring LETTER_QU;
	static const std::wstring LETTER_Q;
	std::wstring m_Letter;
};

};
// namespace WordJewelsEngine
