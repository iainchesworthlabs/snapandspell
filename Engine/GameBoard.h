#pragma once
#include <array>
#include <cstdint>
#include <exception>
#include <locale>
#include <string>
#include "GameBoardIterator.h"
#include "GameBoardMatrix.h"
#include "GameBoardNode.h"
#include "StringConversions.h"

namespace WordJewelsEngine
{

class GameBoard
{
	friend std::wostream& operator<<(std::wostream& out, const GameBoard& gb);
	friend std::wistream& operator>>(std::wistream& out, GameBoard& gb);

public:
	static const uint32_t NUMBER_OF_ROWS_PER_BOARD = 5;
	static const uint32_t NUMBER_OF_COLUMNS_PER_BOARD = 5;

private:
	typedef GameBoardMatrix<GameBoardNode> board_t;

public:
	typedef GameBoardNode          value_type;
	typedef GameBoardNode*         pointer;
	typedef const GameBoardNode*   const_pointer;
	typedef GameBoardNode&         reference;
	typedef const GameBoardNode&   const_reference;
	typedef size_t				   size_type;
	typedef ptrdiff_t			   difference_type;
	typedef value_type			   bool_type;

public:
	typedef const GameBoardIterator const_iterator;
	typedef GameBoardIterator iterator;

public:
	GameBoard();
	GameBoard(const GameBoard& old);
	virtual ~GameBoard();

public:
	GameBoard& operator=(const GameBoard& rhs);

public:
	reference front();
	reference back();
	const_reference front() const ;
	const_reference back() const ;

public:
	const_iterator begin() const;
	const_iterator end() const;
	iterator begin();
	iterator end();

public:
	void clear();

public:
	GameBoardNode& at(uint32_t row, uint32_t col);
	const GameBoardNode& at(uint32_t row, uint32_t col) const;

private:
	void check_item_range(uint32_t row, uint32_t col) const;

public:
	void load(const uint32_t rowNum, std::array<value_type, NUMBER_OF_COLUMNS_PER_BOARD>& posLetters);
	void load(const uint32_t rowNum, value_type posOne, value_type posTwo, value_type posThree, value_type posFour, value_type posFive);

public:
	/*operator bool_type() const
	{
		return false;
	}*/

	friend bool operator==(const GameBoard& lhs, const GameBoard& rhs);
	friend bool operator<(const GameBoard& lhs, const GameBoard& rhs);

public:
	template<class char_t, class char_traits>
    std::basic_ostream<char_t, char_traits>& put(std::basic_ostream<char_t, char_traits>& os) const;

public: 
	template<class char_t, class char_traits>
    std::basic_istream<char_t, char_traits>& get(std::basic_istream<char_t, char_traits>& is);

private:
	board_t m_Board;
};

};
// namespace WordJewelsEngine
