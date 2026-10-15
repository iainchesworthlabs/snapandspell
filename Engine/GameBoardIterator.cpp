#include "pch.h"
#include "GameBoardIterator.h"
#include "GameBoard.h"

namespace WordJewelsEngine
{

GameBoardIterator::GameBoardIterator() : m_pBoard(nullptr), m_RowPosition(0), m_ColPosition(0)
{
}

GameBoardIterator::GameBoardIterator(const GameBoard* pBoard, int64_t row, int64_t col) : m_pBoard(pBoard), m_RowPosition(0), m_ColPosition(0)
{
	if (nullptr == m_pBoard)
	{
		throw std::invalid_argument("GameBoardIterator::GameBoardIterator - invalid board object");
	}

	if ((0 > m_RowPosition) || (GameBoard::NUMBER_OF_ROWS_PER_BOARD <= m_RowPosition))
	{
		throw std::out_of_range("GameBoardIterator::GameBoardIterator - invalid row position");
	}

	if ((0 > m_ColPosition) || (GameBoard::NUMBER_OF_COLUMNS_PER_BOARD <= m_ColPosition))
	{
		throw std::out_of_range("GameBoardIterator::GameBoardIterator - invalid column position");
	}

	m_RowPosition = static_cast<uint32_t>(row);
	m_ColPosition = static_cast<uint32_t>(col);
}

GameBoardIterator& GameBoardIterator::operator++()
{
	// Increment the current position in the row.
	++m_RowPosition;

	// If outside the row, incrememnt the column
	if (GameBoard::NUMBER_OF_ROWS_PER_BOARD == m_RowPosition)
	{
		++m_ColPosition;
	}

	// If outside the column, reset the column back to the start.
	if (GameBoard::NUMBER_OF_COLUMNS_PER_BOARD != m_ColPosition)
	{
		if (GameBoard::NUMBER_OF_ROWS_PER_BOARD != m_RowPosition)
		{
			// Not at the end of the row, don't reset yet.
		}
		else
		{	
			// Not yet at end so reset the row.
			m_RowPosition = 0;
		}
	}
	else
	{
		// At ::end()...do nothing.
	}

	return *this;
}

GameBoardIterator& GameBoardIterator::operator--()
{
	///FIXME
	return *this;
}

GameBoardIterator& GameBoardIterator::operator++(int)
{
	///FIXME
	return *this;
}

GameBoardIterator& GameBoardIterator::operator--(int)
{
	///FIXME
	return *this;
}

GameBoardIterator GameBoardIterator::operator+(const difference_type& n) const
{
	///FIXME
	return *this;
}

GameBoardIterator& GameBoardIterator::operator+=(const difference_type& n)
{
	///FIXME
	return *this;
}

GameBoardIterator GameBoardIterator::operator-(const difference_type& n) const
{
	///FIXME
	return *this;
}

GameBoardIterator& GameBoardIterator::operator-=(const difference_type& n)
{
	///FIXME
	return *this;
}

GameBoardIterator::reference GameBoardIterator::operator*() 
{
	return const_cast<reference>(m_pBoard->at(m_RowPosition, m_ColPosition)); 
}

const GameBoardIterator::reference GameBoardIterator::operator*() const 
{
	return const_cast<const reference> (m_pBoard->at(m_RowPosition, m_ColPosition)); 
}

GameBoardIterator::pointer GameBoardIterator::operator->() 
{ 
	return const_cast<pointer>(&m_pBoard->at(m_RowPosition, m_ColPosition));
}

GameBoardIterator::pointer GameBoardIterator::operator->() const 
{ 
	return const_cast<pointer>(&m_pBoard->at(m_RowPosition, m_ColPosition)); 
}

const uint32_t GameBoardIterator::Row() const
{
	return m_RowPosition;
}

const uint32_t GameBoardIterator::Column() const
{
	return m_ColPosition;
}

bool operator==(const GameBoardIterator& r1, const GameBoardIterator& r2)
{ 
	bool inSameRow(r1.m_RowPosition == r2.m_RowPosition);
	bool inSameCol(r1.m_ColPosition == r2.m_ColPosition);
	bool sameBoard(r1.m_pBoard == r2.m_pBoard);

	return (inSameRow && inSameCol && sameBoard); 
}

bool operator!=(const GameBoardIterator& r1, const GameBoardIterator& r2)
{ 
	return (!operator==(r1, r2)); 
}

};
// namespace WordJewelsEngine
