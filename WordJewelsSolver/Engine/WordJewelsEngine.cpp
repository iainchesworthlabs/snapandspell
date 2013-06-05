#include "Stdafx.h"
#include "WordJewelsEngine.h"

namespace WordJewelsEngine
{
	
Engine::Engine() :
	m_Dictionary(),
	m_Searcher(),
	m_Gameboard(),
	m_DiscoveredWords(),
	m_Rules()
{
}

Engine::~Engine()
{
}

}
// namespace WordJewelsEngine