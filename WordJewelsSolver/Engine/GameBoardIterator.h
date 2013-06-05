#pragma once
#include <cstdint>
#include <exception>
#include <iterator>

namespace WordJewelsEngine
{

// Forward declarations
class GameBoard;
class GameBoardNode;

class GameBoardIterator : public std::iterator<std::random_access_iterator_tag, GameBoardNode>
{
protected:
	const GameBoard* m_pBoard;
	uint32_t m_RowPosition;
	uint32_t m_ColPosition;

public:
	typedef iterator						self_type;
	typedef GameBoardNode					value_type;
	typedef GameBoardNode&					reference;
	typedef GameBoardNode*					pointer;
	typedef std::random_access_iterator_tag iterator_category;

public:
	GameBoardIterator();
	GameBoardIterator(const GameBoard* pBoard, int64_t row = 0, int64_t col = 0);

public:
	GameBoardIterator& operator++();
	GameBoardIterator& operator--();
	GameBoardIterator& operator++(int);
	GameBoardIterator& operator--(int);

public:
	GameBoardIterator operator+(const difference_type& n) const;
	GameBoardIterator& operator+=(const difference_type& n);
	GameBoardIterator operator-(const difference_type& n) const;
	GameBoardIterator& operator-=(const difference_type& n);

public:
	reference operator*() ;
	const reference operator*() const;
    pointer operator->();
    pointer operator->() const;

public:
	const uint32_t Row() const;
	const uint32_t Column() const;

private:
	friend bool operator==(const GameBoardIterator& r1, const GameBoardIterator& r2);
	friend bool operator!=(const GameBoardIterator& r1, const GameBoardIterator& r2);
};

bool operator==(const GameBoardIterator& r1, const GameBoardIterator& r2);
bool operator!=(const GameBoardIterator& r1, const GameBoardIterator& r2);

};
// namespace WordJewelsEngine
