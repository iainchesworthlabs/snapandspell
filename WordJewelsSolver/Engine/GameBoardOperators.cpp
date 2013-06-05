#include "Stdafx.h"
#include "GameBoardOperators.h"

namespace WordJewelsEngine
{

bool operator==(const GameBoard& lhs, const GameBoard& rhs)
{
	return (lhs.m_Board == rhs.m_Board);
}

bool operator<(const GameBoard& lhs, const GameBoard& rhs)
{
	return false;
}

bool operator>(const GameBoard& lhs, const GameBoard& rhs) 
{ 
	return (rhs < lhs); 
}

bool operator<=(const GameBoard& lhs, const GameBoard& rhs) 
{ 
	return !(rhs < lhs); 
}

bool operator>=(const GameBoard& lhs, const GameBoard& rhs) 
{ 
	return !(lhs < rhs); 
}

bool operator!=(const GameBoard& lhs, const GameBoard& rhs) 
{ 
	return !(lhs == rhs); 
}

}
// namespace WordJewelsEngine
