#pragma once

#include "mclo/container/detail/packed_int_base.hpp"

#include <initializer_list>
#include <ranges>
#include <vector>

namespace mclo
{
	/// @brief A bit-packed integer vector storing virtual integers of a fixed bit width within physical storage
	/// @details Stores M virtual objects of bit width BitWidth packed contiguously into P physical objects of type
	/// UnderlyingType. Each virtual integer occupies exactly BitWidth bits and may span across physical element
	/// boundaries. All access operations are O(1).
	/// @tparam BitWidth Number of bits per virtual integer, must be in range [1, bits per UnderlyingType]
	/// @tparam UnderlyingType Unsigned integer type used for physical storage
	/// @tparam UnderlyingContainer Vector-like container of UnderlyingType used for storage
	template <std::size_t BitWidth,
			  std::unsigned_integral UnderlyingType = std::size_t,
			  std::ranges::contiguous_range UnderlyingContainer = std::vector<UnderlyingType>>
	class packed_int_vector
		: public detail::packed_int_base<BitWidth,
										 UnderlyingType,
										 typename UnderlyingContainer::size_type,
										 packed_int_vector<BitWidth, UnderlyingType, UnderlyingContainer>>
	{
		using base = detail::packed_int_base<BitWidth,
											 UnderlyingType,
											 typename UnderlyingContainer::size_type,
											 packed_int_vector<BitWidth, UnderlyingType, UnderlyingContainer>>;
		friend base;

		static_assert( std::is_same_v<UnderlyingType, typename UnderlyingContainer::value_type>,
					   "UnderlyingType must match the container's value_type" );

	public:
		using typename base::size_type;
		using typename base::underlying_type;
		using typename base::value_type;
		using underlying_container = UnderlyingContainer;

		using base::bit_width;
		using base::max_value;

		/// @brief Default construct an empty packed_int_vector
		packed_int_vector() noexcept = default;

		/// @brief Construct a packed_int_vector with the given number of zero-initialized virtual elements
		/// @param size Number of virtual integers to store
		explicit packed_int_vector( const size_type size )
			: m_size( size )
		{
			m_container.resize( base::required_physical_size( size ) );
		}

		/// @brief Construct a packed_int_vector with the given number of virtual elements filled with a value
		/// @param size Number of virtual integers to store
		/// @param value Value to fill each virtual integer with
		packed_int_vector( const size_type size, const value_type value )
			: packed_int_vector( size )
		{
			this->fill( value );
		}

		/// @brief Construct a packed_int_vector from an initializer list of values
		/// @param init List of values to store
		packed_int_vector( const std::initializer_list<value_type> init )
			: packed_int_vector( static_cast<size_type>( init.size() ) )
		{
			size_type i = 0;
			for ( const auto value : init )
			{
				this->set( i++, value );
			}
		}

		/// @brief Append a virtual integer to the end
		/// @param value Value to append
		void push_back( const value_type value )
		{
			DEBUG_ASSERT( value <= max_value, "Value exceeds maximum for BitWidth" );

			const size_type current_size = m_container.size();
			const size_type needed = base::required_physical_size( m_size + 1 );
			if ( needed > current_size )
			{
				// Use push_back for amortized O(1) growth via the underlying container's
				// capacity strategy, rather than resize(needed) which allocates exactly.
				// When padding is required and the container is empty, the first allocation
				// needs multiple elements (data + padding), so we fall back to resize.
				if constexpr ( base::padding > 0 )
				{
					if ( current_size == 0 )
					{
						m_container.resize( needed );
					}
					else
					{
						m_container.push_back( underlying_type{ 0 } );
					}
				}
				else
				{
					m_container.push_back( underlying_type{ 0 } );
				}
			}

			++m_size;
			this->set( m_size - 1, value );
		}

		/// @brief Remove the last virtual integer
		void pop_back() noexcept
		{
			DEBUG_ASSERT( !this->empty(), "Container is empty" );
			--m_size;
		}

		/// @brief Get the number of virtual integers that can be held without reallocation
		[[nodiscard]] size_type capacity() const noexcept
		{
			const auto cap = m_container.capacity();
			if ( cap <= base::padding )
			{
				return 0;
			}
			return static_cast<size_type>( ( ( cap - base::padding ) * sizeof( underlying_type ) * CHAR_BIT ) /
										   bit_width );
		}

		/// @brief Get the maximum number of virtual integers this container could hold
		[[nodiscard]] size_type max_size() const noexcept
		{
			static constexpr size_type max_virtual_size = std::numeric_limits<size_type>::max() / bit_width;
			const size_type container_limit = static_cast<size_type>(
				( m_container.max_size() - base::padding ) * sizeof( underlying_type ) * CHAR_BIT / bit_width );
			return max_virtual_size < container_limit ? max_virtual_size : container_limit;
		}

		/// @brief Reserve storage for at least the given number of virtual integers
		/// @param new_cap Minimum virtual capacity to reserve
		void reserve( const size_type new_cap )
		{
			m_container.reserve( base::required_physical_size( new_cap ) );
		}

		/// @brief Resize the container to hold the given number of virtual integers
		/// @details New elements are zero-initialized. Existing elements are preserved.
		/// @param new_size New number of virtual integers
		void resize( const size_type new_size )
		{
			DEBUG_ASSERT( new_size <= max_size(), "Virtual size exceeds max_size" );
			if ( new_size > m_size )
			{
				m_container.resize( base::required_physical_size( new_size ) );
			}
			m_size = new_size;
		}

		/// @brief Resize the container filling new elements with the given value
		/// @param new_size New number of virtual integers
		/// @param value Value to fill new elements with
		void resize( const size_type new_size, const value_type value )
		{
			const size_type old_size = m_size;
			resize( new_size );

			for ( size_type i = old_size; i < m_size; ++i )
			{
				this->set( i, value );
			}
		}

		/// @brief Reduce physical capacity to fit the current size
		void shrink_to_fit()
		{
			m_container.resize( base::required_physical_size( m_size ) );
			m_container.shrink_to_fit();
		}

		/// @brief Remove all virtual integers without changing capacity
		void clear() noexcept
		{
			m_size = 0;
		}

		void swap( packed_int_vector& other ) noexcept
		{
			using std::swap;
			swap( m_container, other.m_container );
			swap( m_size, other.m_size );
		}

		friend void swap( packed_int_vector& lhs, packed_int_vector& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

	private:
		// CRTP derived interface
		constexpr underlying_type* derived_data() noexcept
		{
			return m_container.data();
		}
		constexpr const underlying_type* derived_data() const noexcept
		{
			return m_container.data();
		}
		constexpr size_type derived_size() const noexcept
		{
			return m_size;
		}
		constexpr size_type derived_physical_size() const noexcept
		{
			return m_container.size();
		}

		underlying_container m_container;
		size_type m_size = 0;
	};
}
