#pragma once

#include "mclo/debug/assert.hpp"

#include <concepts>
#include <iterator>
#include <limits>
#include <type_traits>

namespace mclo
{
	inline constexpr std::size_t dynamic_extent = std::numeric_limits<std::size_t>::max();

	template <typename T, std::size_t Extent = dynamic_extent>
	class span;

	namespace detail
	{
		template <typename T, std::size_t Extent>
		struct span_base
		{
			constexpr span_base() noexcept = default;
			constexpr span_base( T* const data, const std::size_t ) noexcept
				: m_data( data )
			{
			}

			T* m_data = nullptr;
			static constexpr std::size_t m_size = Extent;
		};

		template <typename T>
		struct span_base<T, dynamic_extent>
		{
			constexpr span_base() noexcept = default;
			constexpr span_base( T* const data, const std::size_t size ) noexcept
				: m_data( data )
				, m_size( size )
			{
			}

			T* m_data = nullptr;
			std::size_t m_size = 0;
		};

		template <typename Range>
		constexpr bool banned_span_conversion = std::is_array_v<Range>;

		template <typename T, std::size_t N>
		constexpr bool banned_span_conversion<std::array<T, N>> = true;

		template <typename T, std::size_t N>
		constexpr bool banned_span_conversion<mclo::span<T, N>> = true;

		template <typename From, typename To>
		constexpr bool valid_pointer_conversion = std::is_convertible_v<From ( * )[], To ( * )[]>;

		template <typename It, typename T>
		concept span_compatible_iterator =
			std::contiguous_iterator<It> &&
			valid_pointer_conversion<std::remove_reference_t<std::iter_reference_t<It>>, T>;

		// clang-format off
		template <typename Range, typename T>
		concept span_compatible_range = std::ranges::contiguous_range<Range>
			&&  std::ranges::sized_range<Range>
			&& ( std::is_const_v<T> || std::ranges::borrowed_range<Range> )
			&& (!banned_span_conversion<std::remove_cvref_t<Range>> )
			&& valid_pointer_conversion<std::ranges::range_reference_t<Range>, T>;
		// clang-format on
	}

	template <typename T, std::size_t Extent>
	class span : private detail::span_base<T, Extent>
	{
	private:
		using base = detail::span_base<T, Extent>;
		using base::m_data;
		using base::m_size;

	public:
		using element_type = T;
		using value_type = std::remove_cv_t<T>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using const_pointer = const T*;
		using reference = T&;
		using const_reference = const T&;
		using iterator = pointer;
		using reverse_iterator = std::reverse_iterator<iterator>;

		static constexpr std::size_t extent = Extent;

		constexpr span() noexcept
			requires( extent == 0 || extent == dynamic_extent )
		= default;

		template <detail::span_compatible_iterator<T> It>
		explicit( extent != dynamic_extent ) constexpr span( const It first, const size_type count ) MCLO_NOEXCEPT_TESTS
			: base( std::to_address( first ), count )
		{
			if constexpr ( extent != dynamic_extent )
			{
				DEBUG_ASSERT( count == extent, "Count was not actually equal to static extent" );
			}
		}

		template <detail::span_compatible_iterator<T> It, std::sized_sentinel_for<It> Sentinel>
		explicit( extent != dynamic_extent ) constexpr span( const It first, const Sentinel last )
			MCLO_NOEXCEPT_TESTS_IF( noexcept( last - first ) )
			: base( std::to_address( first ), static_cast<size_type>( last - first ) )
		{
			if constexpr ( extent != dynamic_extent )
			{
				DEBUG_ASSERT( static_cast<size_type>( last - first ) == extent,
							  "Iterator pair distance was not actually equal to static extent" );
			}
		}

		template <std::size_t N>
			requires( extent == dynamic_extent || N == extent )
		constexpr span( std::type_identity_t<element_type> ( &arr )[ N ] ) noexcept
			: base( arr, N )
		{
		}

		template <typename U, std::size_t N>
			requires( ( extent == dynamic_extent || N == extent ) && detail::valid_pointer_conversion<U, element_type> )
		constexpr span( std::array<U, N>& arr ) noexcept
			: base( arr, N )
		{
		}

		template <typename U, std::size_t N>
			requires( ( extent == dynamic_extent || N == extent ) &&
					  detail::valid_pointer_conversion<const U, element_type> )
		constexpr span( const std::array<U, N>& arr ) noexcept
			: base( arr, N )
		{
		}

		template <detail::span_compatible_range<element_type> Range>
		explicit( extent != dynamic_extent ) constexpr span( Range&& range )
			: base( std::ranges::data( range ), std::ranges::size( range ) )
		{
			if constexpr ( extent != dynamic_extent )
			{
				DEBUG_ASSERT( std::ranges::size( range ) == extent,
							  "Range distance was not actually equal to static extent" );
			}
		}

		explicit( extent !=
				  dynamic_extent ) constexpr span( std::initializer_list<value_type> init_list ) MCLO_NOEXCEPT_TESTS
			requires( std::is_const_v<element_type> )
			: base( init_list.begin(), init_list.size() )
		{
			if constexpr ( extent != dynamic_extent )
			{
				DEBUG_ASSERT( init_list.size() == extent, "Range distance was not actually equal to static extent" );
			}
		}

		template <typename U, std::size_t N>
			requires( ( extent == dynamic_extent || N == dynamic_extent || extent == N ) &&
					  detail::valid_pointer_conversion<U, element_type> )
		explicit( extent != dynamic_extent &&
				  N == dynamic_extent ) constexpr span( const span<U, N>& other ) MCLO_NOEXCEPT_TESTS
			: base( other.data(), other.size() )
		{
			if constexpr ( extent != dynamic_extent )
			{
				DEBUG_ASSERT( other.size() == extent, "Other span size was not actually equal to static extent" );
			}
		}

		constexpr span( const span& other ) noexcept = default;
		constexpr span& operator=( const span& other ) noexcept = default;

		[[nodiscard]] constexpr reference front() const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( m_size != 0, "Span is empty" );
			return *m_data;
		}

		[[nodiscard]] constexpr reference back() const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( m_size != 0, "Span is empty" );
			return m_data[ m_size - 1 ];
		}

		[[nodiscard]] constexpr reference operator[]( const size_type index ) const
		{
			DEBUG_ASSERT( index < m_size, "Index out of bounds" );
			return m_data[ index ];
		}

		[[nodiscard]] constexpr pointer data() const noexcept
		{
			return m_data;
		}

		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return m_size;
		}

		[[nodiscard]] constexpr size_type size_bytes() const noexcept
		{
			return m_size * sizeof( element_type );
		}

		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return m_size == 0;
		}

		template <std::size_t Count>
		[[nodiscard]] constexpr auto first() const MCLO_NOEXCEPT_TESTS
		{
			if constexpr ( extent != dynamic_extent )
			{
				static_assert( Count <= Extent, "Count is out of range" );
			}
			else
			{
				DEBUG_ASSERT( Count <= m_size, "Count is out of range" );
			}
			return span<element_type, Count>( m_data, Count );
		}

		[[nodiscard]] constexpr auto first( const size_type count ) const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( count <= m_size, "Count is out of range" );
			return span<element_type, dynamic_extent>( m_data, count );
		}

		template <std::size_t Count>
		[[nodiscard]] constexpr auto last() const MCLO_NOEXCEPT_TESTS
		{
			if constexpr ( extent != dynamic_extent )
			{
				static_assert( Count <= Extent, "Count is out of range" );
			}
			else
			{
				DEBUG_ASSERT( Count <= m_size, "Count is out of range" );
			}
			return span<element_type, Count>( m_data + ( m_size - Count ), Count );
		}

		[[nodiscard]] constexpr auto last( const size_type count ) const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( count <= m_size, "Count is out of range" );
			return span<element_type, dynamic_extent>( m_data + ( m_size - count ), count );
		}

		template <std::size_t Offset, std::size_t Count = dynamic_extent>
		[[nodiscard]] constexpr auto subspan() const MCLO_NOEXCEPT_TESTS
		{
			if constexpr ( extent != dynamic_extent )
			{
				static_assert( Offset <= extent, "Offset is out of range" );
				static_assert( Count == dynamic_extent || Count <= extent - Offset, "Count is out of range in" );
			}
			else
			{
				DEBUG_ASSERT( Offset <= m_size, "Offset is out of range" );
				if constexpr ( Count != dynamic_extent )
				{
					DEBUG_ASSERT( Count <= m_size - Offset, "Count is out of range" );
				}
			}
			using return_type =
				span<element_type,
					 Count != dynamic_extent ? Count : ( extent != dynamic_extent ? extent - Offset : dynamic_extent )>;
			return return_type( m_data + Offset, Count == dynamic_extent ? m_size - Offset : Count );
		}

		[[nodiscard]] constexpr auto subspan( const size_type offset,
											  const size_type count = dynamic_extent ) const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( offset <= m_size, "Offset is out of range" );
			DEBUG_ASSERT( count == dynamic_extent || count <= m_size - offset, "Count is out of range" );
			using return_type = span<element_type, dynamic_extent>;
			return return_type( m_data + offset, count == dynamic_extent ? m_size - offset : count );
		}

		[[nodiscard]] constexpr iterator begin() const noexcept
		{
			return m_data;
		}
		[[nodiscard]] constexpr iterator end() const noexcept
		{
			return m_data + m_size;
		}

		[[nodiscard]] constexpr iterator rbegin() const noexcept
		{
			return std::make_reverse_iterator( end() );
		}
		[[nodiscard]] constexpr iterator rend() const noexcept
		{
			return std::make_reverse_iterator( begin() );
		}
	};

	template <typename It, typename EndOrSize>
	span( It, EndOrSize ) -> span<std::remove_reference_t<std::iter_reference_t<It>>>;

	template <typename T, std::size_t N>
	span( T ( & )[ N ] ) -> span<T, N>;

	template <typename T, std::size_t N>
	span( std::array<T, N>& ) -> span<T, N>;

	template <typename T, std::size_t N>
	span( const std::array<T, N>& ) -> span<const T, N>;

	template <std::ranges::contiguous_range Range>
	span( Range&& ) -> span<std::remove_reference_t<std::ranges::range_reference_t<Range>>>;

	template <typename T, std::size_t N>
	span<const std::byte, N == dynamic_extent ? dynamic_extent : N * sizeof( T )> as_bytes(
		const span<T, N> s ) noexcept
	{
		return { reinterpret_cast<const std::byte*>( s.data() ), s.size_bytes() };
	}

	template <typename T, std::size_t N>
	span<std::byte, N == dynamic_extent ? dynamic_extent : N * sizeof( T )> as_writable_bytes(
		const span<T, N> s ) noexcept
	{
		return { reinterpret_cast<std::byte*>( s.data() ), s.size_bytes() };
	}
}

namespace std::ranges
{
	template <typename T, std::size_t Extent>
	constexpr bool enable_borrowed_range<mclo::span<T, Extent>> = true;

	template <typename T, std::size_t Extent>
	constexpr bool enable_view<mclo::span<T, Extent>> = true;
}
