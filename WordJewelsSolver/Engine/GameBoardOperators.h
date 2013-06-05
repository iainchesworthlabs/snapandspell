#pragma once
#include "GameBoard.h"

namespace WordJewelsEngine
{

bool operator==(const GameBoard& lhs, const GameBoard& rhs);
bool operator<(const GameBoard& lhs, const GameBoard& rhs);
bool operator>(const GameBoard& lhs, const GameBoard& rhs);
bool operator<=(const GameBoard& lhs, const GameBoard& rhs);
bool operator>=(const GameBoard& lhs, const GameBoard& rhs);
bool operator!=(const GameBoard& lhs, const GameBoard& rhs);

}
// namespace WordJewelsEngine
