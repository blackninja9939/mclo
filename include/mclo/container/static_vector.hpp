#pragma once

#include "mclo/container/detail/inline_buffer_vector.hpp"

namespace mclo
{
	/// @brief static_vector size independent code, mimics the API of std::vector
	/// @details Can not be constructed directly should only be used as a reference type in functions
	/// which should operate on any sized static_vector of a given type.
	/// @tparam T Type of objects stored
	template <typename T>
	using static_vector_base = detail::inline_buffer_vector_base<T, false>;

	/// @brief static_vector stores Capacity elements inline on the stack, it can NOT grow and allocate on the heap,
	/// mirrors std::vector API
	/// @details static_vector_base<T> provides a size erased type that provides all the same functions for code generic
	/// over size
	/// @tparam T Type of objects stored
	/// @tparam Capacity Number of elements to store inline, if not provided defaults to try and keep the overall stack
	/// size to one cache line (64 bytes)
	template <typename T, std::uint32_t Capacity = detail::default_inline_capacity<T>>
	using static_vector = detail::inline_buffer_vector<T, false, Capacity>;
}
