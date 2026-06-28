#pragma once

#include "mclo/container/detail/packed_int_base.hpp"

#include <array>
#include <initializer_list>

namespace mclo
{
	/// @brief A bit-packed integer array storing a fixed number of virtual integers of a fixed bit width
	/// @details Stores Size virtual objects of bit width BitWidth packed contiguously into physical objects of type
	/// UnderlyingType. Each virtual integer occupies exactly BitWidth bits and may span across physical element
	/// boundaries. All access operations are O(1). Fully constexpr.
	/// @tparam BitWidth Number of bits per virtual integer, must be in range [1, bits per UnderlyingType]
	/// @tparam Size Number of virtual integers to store
	/// @tparam UnderlyingType Unsigned integer type used for physical storage
	template <std::size_t BitWidth, std::size_t Size, std::unsigned_integral UnderlyingType = std::size_t>
	class packed_int_array
		: public detail::
			  packed_int_base<BitWidth, UnderlyingType, std::size_t, packed_int_array<BitWidth, Size, UnderlyingType>>
	{
		using base = detail::
			packed_int_base<BitWidth, UnderlyingType, std::size_t, packed_int_array<BitWidth, Size, UnderlyingType>>;
		friend base;

	public:
		using typename base::size_type;
		using typename base::underlying_type;
		using typename base::value_type;

		using base::bit_width;
		using base::max_value;

		/// @brief Default construct a zero-initialized packed_int_array
		constexpr packed_int_array() noexcept = default;

		/// @brief Construct a packed_int_array with all elements filled with the given value
		/// @param value Value to fill each virtual integer with
		constexpr explicit packed_int_array( const value_type value ) noexcept
		{
			this->fill( value );
		}

		/// @brief Construct a packed_int_array from an initializer list of values
		/// @param init List of values to store, must have exactly Size elements
		constexpr packed_int_array( const std::initializer_list<value_type> init ) noexcept
		{
			MCLO_DEBUG_ASSERT( init.size() == Size, "Initializer list size must match array Size" );
			size_type i = 0;
			for ( const auto value : init )
			{
				this->set( i++, value );
			}
		}

		constexpr void swap( packed_int_array& other ) noexcept
		{
			using std::swap;
			swap( m_storage, other.m_storage );
		}

		friend constexpr void swap( packed_int_array& lhs, packed_int_array& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

	private:
		static constexpr size_type physical_size = base::required_physical_size( Size );

		// CRTP derived interface
		constexpr underlying_type* derived_data() noexcept
		{
			return m_storage.data();
		}
		constexpr const underlying_type* derived_data() const noexcept
		{
			return m_storage.data();
		}
		constexpr size_type derived_size() const noexcept
		{
			return Size;
		}
		constexpr size_type derived_physical_size() const noexcept
		{
			return physical_size;
		}

		std::array<underlying_type, physical_size> m_storage{};
	};
}
