#include "Stdafx.h"
#include "GameBoard.h"

namespace WordJewelsEngine
{

GameBoard::GameBoard() : m_Board(NUMBER_OF_ROWS_PER_BOARD, NUMBER_OF_COLUMNS_PER_BOARD, GameBoardNode::EMPTY_NODE)
{
}	
GameBoard::GameBoard(const GameBoard& old) : m_Board(old.m_Board)
{
}
GameBoard::~GameBoard()
{
}

GameBoard& GameBoard::operator=(const GameBoard& rhs)
{
	if (this != &rhs)
	{
		m_Board = rhs.m_Board;
	}

	return *this;
}

GameBoard::reference GameBoard::front() 
{ 
	return m_Board.item(0, 0); 
}
GameBoard::reference GameBoard::back() 
{ 
	return m_Board.item(NUMBER_OF_ROWS_PER_BOARD-1, NUMBER_OF_COLUMNS_PER_BOARD-1); 
}
GameBoard::const_reference GameBoard::front() const 
{ 
	return m_Board.item(0, 0); 
}
GameBoard::const_reference GameBoard::back() const 
{ 
	return m_Board.item(NUMBER_OF_ROWS_PER_BOARD-1, NUMBER_OF_COLUMNS_PER_BOARD-1); 
}

GameBoard::const_iterator GameBoard::begin() const
{
	return const_iterator(this);
}
GameBoard::const_iterator GameBoard::end() const
{
	return const_iterator(this, NUMBER_OF_ROWS_PER_BOARD-1, NUMBER_OF_COLUMNS_PER_BOARD-1);
}
GameBoard::iterator GameBoard::begin()
{
	return iterator(this);
}
GameBoard::iterator GameBoard::end()
{
	return iterator(this, NUMBER_OF_ROWS_PER_BOARD-1, NUMBER_OF_COLUMNS_PER_BOARD-1);
}

void GameBoard::clear()
{
	m_Board.fill(GameBoardNode::EMPTY_NODE);
}

GameBoardNode& GameBoard::at(uint32_t row, uint32_t col)
{
	check_item_range(row, col);
	return const_cast<GameBoardNode&>(m_Board.item(row, col));
}

const GameBoardNode& GameBoard::at(uint32_t row, uint32_t col) const
{
	check_item_range(row, col);
	return m_Board.item(row, col);
}

void GameBoard::check_item_range(uint32_t row, uint32_t col) const
{
	if ((0 > row) || (NUMBER_OF_ROWS_PER_BOARD <= row))
	{
		throw std::out_of_range("GameBoard::at - row value is invalid");
	}
	else if ((0 > col) || (NUMBER_OF_COLUMNS_PER_BOARD <= col))
	{
		throw std::out_of_range("GameBoard::at - column value is invalid");
	}
	else
	{
		// Do nothing...values are valid.
	}
}

void GameBoard::load(const uint32_t rowNum, std::array<value_type, NUMBER_OF_COLUMNS_PER_BOARD> posLetters)
{
	if (0 > rowNum)
	{
		throw std::out_of_range("GameBoard::load - row number is too small");
	}
	else if (NUMBER_OF_ROWS_PER_BOARD <= rowNum)
	{
		throw std::out_of_range("GameBoard::load - row number is too large");
	}
	else 
	{
		m_Board(rowNum, 0) = posLetters[0];
		m_Board(rowNum, 1) = posLetters[1];
		m_Board(rowNum, 2) = posLetters[2];
		m_Board(rowNum, 3) = posLetters[3];
		m_Board(rowNum, 4) = posLetters[4];
	}
}

void GameBoard::load(const uint32_t rowNum, value_type posOne, value_type posTwo, value_type posThree, value_type posFour, value_type posFive)
{
	if (0 > rowNum)
	{
		throw std::out_of_range("GameBoard::load - row number is too small");
	}
	else if (NUMBER_OF_ROWS_PER_BOARD <= rowNum)
	{
		throw std::out_of_range("GameBoard::load - row number is too large");
	}
	else 
	{
		m_Board(rowNum, 0) = posOne;
		m_Board(rowNum, 1) = posTwo;
		m_Board(rowNum, 2) = posThree;
		m_Board(rowNum, 3) = posFour;
		m_Board(rowNum, 4) = posFive;
	}
}

std::wostream& operator<<(std::wostream& out, const GameBoard& gb)
{
	return out;
}

std::wistream& operator>>(std::wistream& in, GameBoard& gb)
{
	std::array<GameBoard::value_type, GameBoard::NUMBER_OF_COLUMNS_PER_BOARD> rowToLoad;
	static uint32_t streamingRow(0);
	std::wstring newLetterNode;

	for(uint32_t i = 0; i < GameBoard::NUMBER_OF_COLUMNS_PER_BOARD; ++i)
	{
		in >> newLetterNode;
		rowToLoad[i] = newLetterNode;
	}

	// Fine - blat the row into the game board.
	gb.load(streamingRow, rowToLoad);

	if (0 == (++streamingRow % GameBoard::NUMBER_OF_ROWS_PER_BOARD))
	{
		// Reset the streaming row back to the start.
		streamingRow = 0;
	}

	return in;
}

};
// namespace WordJewelsEngine
