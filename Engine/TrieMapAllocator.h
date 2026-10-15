#pragma once

#include <type_traits>
#include "DebugStream.h"

template <typename T>
struct trie_map_allocator
{
	using value_type = T;

	using propagate_on_container_copy_assignment = std::true_type; // for consistency
	using propagate_on_container_move_assignment = std::true_type; // to avoid the pessimization
	using propagate_on_container_swap = std::true_type; // to avoid the undefined behavior

	// to get the C++17 optimization: add this line for non-empty allocators which are always equal
	// using is_always_equal = std::true_type;

	trie_map_allocator() 
	{ 
		wdebugstream() << TEXT("Allocating memory for the trie_map") << std::endl;
	}

	template <class U>
	trie_map_allocator(const trie_map_allocator<U>&) { throw; }

	T* allocate(std::size_t n) { throw; }
	void deallocate(T* ptr, std::size_t n) { throw; }
};

template <typename T, typename U>
bool operator==(const trie_map_allocator<T>&, const trie_map_allocator<U>&) { throw; }

template <typename T, typename U>
bool operator!=(const trie_map_allocator<T>&, const trie_map_allocator<U>&) { throw; }
