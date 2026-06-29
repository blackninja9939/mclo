#pragma once

#include "mclo/container/span.hpp"
#include "mclo/debug/assert.hpp"
#include "mclo/random/weighted_random.hpp"

#include <concepts>
#include <random>
#include <utility>
#include <vector>

namespace mclo
{
	/// @brief A growable collection of elements each paired with a sampling weight, stored as parallel containers.
	/// @details Keeps the elements and their integral weights in two side-by-side containers, in the manner of
	/// @c std::flat_map, so the parallel invariant cannot be violated by callers. Built on top of @c weighted_index,
	/// it provides weighted random selection along with destructive sampling without replacement for shrinking a
	/// sample space. The total weight is cached and maintained incrementally as elements are added and removed.
	/// @tparam T The element type stored in the container.
	/// @tparam Weight The integral weight type associated with each element.
	/// @tparam ValueContainer The contiguous container type holding the elements.
	/// @tparam WeightContainer The contiguous container type holding the weights.
	template <typename T,
			  std::integral Weight = int,
			  std::ranges::contiguous_range ValueContainer = std::vector<T>,
			  std::ranges::contiguous_range WeightContainer = std::vector<Weight>>
	class weighted_vector
	{
	public:
		/// @brief The container type holding the elements.
		using value_container = ValueContainer;

		/// @brief The container type holding the weights.
		using weight_container = WeightContainer;

		/// @brief The element type stored in the container.
		using value_type = typename value_container::value_type;

		/// @brief The integral weight type associated with each element.
		using weight_type = typename weight_container::value_type;

		/// @brief The unsigned type used for sizes and indices.
		using size_type = typename value_container::size_type;

		/// @brief A reference to an element.
		using reference = typename value_container::reference;

		/// @brief A const reference to an element.
		using const_reference = typename value_container::const_reference;

		/// @brief Reserves storage for at least @p count elements to avoid reallocation.
		/// @param count The number of elements to reserve capacity for.
		void reserve( const size_type count )
		{
			m_values.reserve( count );
			m_weights.reserve( count );
		}

		/// @brief Constructs an element in place at the end of the container with the given weight.
		/// @tparam Args The argument types forwarded to the element's constructor.
		/// @param weight The sampling weight for the new element.
		/// @param args The arguments forwarded to the element's constructor.
		/// @return A reference to the newly constructed element.
		/// @pre @p weight must be non-negative.
		template <typename... Args>
		reference emplace_back( const weight_type weight, Args&&... args )
		{
			MCLO_DEBUG_ASSERT( weight >= 0, "Weight must be non-negative" );
			reference element = m_values.emplace_back( std::forward<Args>( args )... );
			m_weights.push_back( weight );
			m_total_weight += weight;
			return element;
		}

		/// @brief Appends an element to the end of the container with the given weight.
		/// @param weight The sampling weight for the new element.
		/// @param value The element to append.
		/// @pre @p weight must be non-negative.
		void push_back( const weight_type weight, const value_type& value )
		{
			emplace_back( weight, value );
		}

		/// @brief Appends an element to the end of the container with the given weight.
		/// @param weight The sampling weight for the new element.
		/// @param value The element to append, moved into the container.
		/// @pre @p weight must be non-negative.
		void push_back( const weight_type weight, value_type&& value )
		{
			emplace_back( weight, std::move( value ) );
		}

		/// @brief Removes all elements from the container.
		void clear() noexcept
		{
			m_values.clear();
			m_weights.clear();
			m_total_weight = 0;
		}

		/// @brief Returns the number of elements in the container.
		/// @return The element count.
		[[nodiscard]] size_type size() const noexcept
		{
			return m_values.size();
		}

		/// @brief Checks whether the container has no elements.
		/// @return @c true if the container is empty.
		[[nodiscard]] bool empty() const noexcept
		{
			return m_values.empty();
		}

		/// @brief Accesses the element at the given index.
		/// @param index The index of the element.
		/// @return A reference to the element.
		/// @pre @p index must be less than @c size().
		[[nodiscard]] reference operator[]( const size_type index ) noexcept
		{
			MCLO_DEBUG_ASSERT( index < size(), "Index out of range" );
			return m_values[ index ];
		}

		/// @brief Accesses the element at the given index.
		/// @param index The index of the element.
		/// @return A const reference to the element.
		/// @pre @p index must be less than @c size().
		[[nodiscard]] const_reference operator[]( const size_type index ) const noexcept
		{
			MCLO_DEBUG_ASSERT( index < size(), "Index out of range" );
			return m_values[ index ];
		}

		/// @brief Returns the weight of the element at the given index.
		/// @param index The index of the element.
		/// @return The element's sampling weight.
		/// @pre @p index must be less than @c size().
		[[nodiscard]] weight_type weight( const size_type index ) const noexcept
		{
			MCLO_DEBUG_ASSERT( index < size(), "Index out of range" );
			return m_weights[ index ];
		}

		/// @brief Sets the weight of the element at the given index.
		/// @param index The index of the element.
		/// @param weight The new sampling weight.
		/// @pre @p index must be less than @c size() and @p weight must be non-negative.
		void set_weight( const size_type index, const weight_type weight ) noexcept
		{
			MCLO_DEBUG_ASSERT( index < size(), "Index out of range" );
			MCLO_DEBUG_ASSERT( weight >= weight_type( 0 ), "Weight must be non-negative" );
			m_total_weight += weight - m_weights[ index ];
			m_weights[ index ] = weight;
		}

		/// @brief Returns a view over the elements.
		/// @return A span over the contained elements.
		[[nodiscard]] mclo::span<value_type> values() noexcept
		{
			return m_values;
		}

		/// @brief Returns a read-only view over the elements.
		/// @return A const span over the contained elements.
		[[nodiscard]] mclo::span<const value_type> values() const noexcept
		{
			return m_values;
		}

		/// @brief Returns a read-only view over the weights.
		/// @return A const span over the contained weights, parallel to @c values().
		[[nodiscard]] mclo::span<const weight_type> weights() const noexcept
		{
			return m_weights;
		}

		/// @brief Returns the sum of all weights.
		/// @return The total weight across every element.
		[[nodiscard]] weight_type total_weight() const noexcept
		{
			return m_total_weight;
		}

		/// @brief Selects a random element index weighted by the stored weights.
		/// @tparam Engine The random bit generator type, satisfying @c std::uniform_random_bit_generator.
		/// @param engine The random bit generator to sample from.
		/// @return The index of the selected element.
		/// @pre The container must not be empty and the total weight must be positive.
		template <std::uniform_random_bit_generator Engine>
		[[nodiscard]] size_type sample( Engine& engine ) const
		{
			return static_cast<size_type>( weighted_index( engine, m_weights, m_total_weight ) );
		}

		/// @brief Removes the element at the given index in constant time without preserving order.
		/// @details Moves the last element into the vacated slot and shrinks the container, so element order is not
		/// preserved. Suitable for a shrinking sample space where ordering is irrelevant.
		/// @param index The index of the element to remove.
		/// @pre @p index must be less than @c size().
		void swap_remove( const size_type index )
		{
			MCLO_DEBUG_ASSERT( index < size(), "Index out of range" );
			m_total_weight -= m_weights[ index ];
			const size_type last = size() - 1;
			if ( index != last )
			{
				m_values[ index ] = std::move( m_values[ last ] );
				m_weights[ index ] = m_weights[ last ];
			}
			m_values.pop_back();
			m_weights.pop_back();
		}

		/// @brief Selects a random element, removes it from the container and returns it.
		/// @details Samples an index, removes it via @c swap_remove and returns the element by move, implementing a
		/// single draw of weighted sampling without replacement.
		/// @tparam Engine The random bit generator type, satisfying @c std::uniform_random_bit_generator.
		/// @param engine The random bit generator to sample from.
		/// @return The sampled element, moved out of the container.
		/// @pre The container must not be empty and the total weight must be positive.
		template <std::uniform_random_bit_generator Engine>
		[[nodiscard]] value_type pop_sample( Engine& engine )
		{
			const size_type index = sample( engine );
			value_type value = std::move( m_values[ index ] );
			swap_remove( index );
			return value;
		}

	private:
		value_container m_values;
		weight_container m_weights;
		weight_type m_total_weight = 0;
	};
}
