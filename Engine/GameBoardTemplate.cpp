#include "pch.h"
#include "GameBoard.h"
#include "GameBoardTemplate.h"

#include <istream>
#include <ostream>

#include "IOState.h"

namespace WordJewelsEngine
{
	
template<class char_t, class char_traits>
std::basic_ostream<char_t, char_traits>& GameBoard::put(std::basic_ostream<char_t, char_traits>& os) const
{
	if (!os.good())
	{
        return os;
	}

    const typename std::basic_ostream<char_t, char_traits>::sentry ok(os);
    if (ok)
    {
		ios_flags_saver flags(os);
        basic_ios_fill_saver<char_t, char_traits> fill(os);

		const std::streamsize width = os.width(0);
        const std::streamsize mysize = 40;

		// right padding
        if (flags.value() & (std::ios_base::right | std::ios_base::internal))
		{
            for (std::streamsize i = width; i > mysize; --i)
			{
                os << fill.value();
			}
		}

		// left padding
		if (flags.value() & std::ios_base::left)
		{
			for (std::streamsize i = width; i > mysize; --i)
			{
				os << fill.value();
			}
		}
	}

	return os;
}

template<class char_t, class char_traits>
std::basic_istream<char_t, char_traits>& GameBoard::get(std::basic_istream<char_t, char_traits>& is)
{
    if (!is.good())
	{
        return is;
	}

	const typename std::basic_istream<char_t, char_traits>::sentry ok(is);
	if (ok)
	{
		if (!is) 
		{
			throw std::runtime_error("failed to extract valid gameboard from stream.");
		}
	}

	return is;
}

template<class char_t, class char_traits>
inline std::basic_ostream<char_t, char_traits>& operator<<(std::basic_ostream<char_t, char_traits>& os, const GameBoard& gameBoard)
{
    return gameBoard.put(os);
}

template<class char_t, class char_traits>
inline std::basic_istream<char_t, char_traits>& operator>>(std::basic_istream<char_t, char_traits>& is, GameBoard& gameBoard)
{
    return gameBoard.get(is);
}

};
// namespace WordJewelsEngine
