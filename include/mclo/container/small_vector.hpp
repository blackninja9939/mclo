#pragma once

#include "mclo/container/detail/inline_buffer_vector.hpp"

namespace mclo
{
	/// @brief small_vector size independent code, mimics the API of std::vector
	/// @details Can not be constructed directly should only be used as a reference type in functions
	/// which should operate on any sized small_vector of a given type.
	/// @tparam T Type of objects stored
	template <typename T>
	using small_vector_base = detail::inline_buffer_vector_base<T, true>;

	/// @brief small_vector stores Capacity elements inline on the stack but can grow and allocate on the heap
	/// additional, mirrors std::vector API
	/// @details small_vector_base<T> provides a size erased type that provides all the same functions for code generic
	/// over size
	/// @tparam T Type of objects stored
	/// @tparam Capacity Number of elements to store inline, if not provided defaults to try and keep the overall stack
	/// size to one cache line (64 bytes)
	template <typename T, std::uint32_t Capacity = detail::default_inline_capacity<T>>
	using small_vector = detail::inline_buffer_vector<T, true, Capacity>;
}
