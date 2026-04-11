#pragma once

#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <ranges>

#include <mclo/debug/assert.hpp>
#include <mclo/numeric/align.hpp>
#include <mclo/numeric/math.hpp>
#include <mclo/preprocessor/platform.hpp>

namespace mclo
{
	template <std::unsigned_integral Size>
	struct flexible_array_header
	{
		Size m_size = 0;
	};

	template <typename T, std::unsigned_integral Size = std::size_t, typename Allocator = std::allocator<T>>
	class flexible_array
	{
		using header_type = flexible_array_header<Size>;

		static constexpr std::size_t required_alignment = std::max( alignof( header_type ), alignof( T ) );

		struct alignas( required_alignment ) aligned_buffer
		{
			std::byte buffer[ required_alignment ];
		};

		using alloc_traits = std::allocator_traits<Allocator>;
		using buffer_allocator = typename alloc_traits::template rebind_alloc<aligned_buffer>;
		using buffer_alloc_traits = std::allocator_traits<buffer_allocator>;

		static_assert( std::is_same_v<typename alloc_traits::pointer, T*>,
					   "flexible_array does not support fancy pointers" );

	public:
		using value_type = T;
		using size_type = Size;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using const_pointer = const T*;
		using reference = T&;
		using const_reference = const T&;
		using allocator_type = Allocator;

		flexible_array() = default;

		explicit flexible_array( const Size size, const Allocator& allocator = Allocator() )
			: m_allocator( allocator )
		{
			if ( size > 0 )
			{
				allocate_header( size );
				std::uninitialized_default_construct_n( element_data(), size );
			}
		}

		explicit flexible_array( const Allocator& allocator ) noexcept
			: m_allocator( allocator )
		{
		}

		template <std::forward_iterator It, std::sentinel_for<It> Sentinel>
			requires std::constructible_from<T, std::iter_reference_t<It>>
		flexible_array( It first, Sentinel last, const Allocator& allocator = Allocator() )
			: m_allocator( allocator )
		{
			const Size count = static_cast<Size>( std::distance( first, last ) );
			if ( count > 0 )
			{
				allocate_header( count );
				std::uninitialized_copy( first, last, element_data() );
			}
		}

		template <std::ranges::forward_range Range>
			requires( !std::same_as<std::remove_cvref_t<Range>, flexible_array> ) &&
					std::constructible_from<T, std::ranges::range_reference_t<Range>>
		flexible_array( Range&& range, const Allocator& allocator = Allocator() )
			: m_allocator( allocator )
		{
			const Size count = static_cast<Size>( std::ranges::distance( range ) );
			if ( count > 0 )
			{
				allocate_header( count );
				std::ranges::uninitialized_copy( range,
												 std::ranges::subrange( element_data(), element_data() + count ) );
			}
		}

		flexible_array( const flexible_array& other )
			: m_allocator( alloc_traits::select_on_container_copy_construction( other.m_allocator ) )
		{
			const Size count = other.size();
			if ( count > 0 )
			{
				allocate_header( count );
				std::uninitialized_copy_n( other.element_data(), count, element_data() );
			}
		}

		flexible_array& operator=( const flexible_array& other )
		{
			if ( this != &other )
			{
				delete_all();
				if constexpr ( alloc_traits::propagate_on_container_copy_assignment::value )
				{
					m_allocator = other.m_allocator;
				}
				const Size count = other.size();
				if ( count > 0 )
				{
					allocate_header( count );
					std::uninitialized_copy_n( other.element_data(), count, element_data() );
				}
			}
			return *this;
		}

		flexible_array( flexible_array&& other ) noexcept
			: m_allocator( std::move( other.m_allocator ) )
			, m_header( std::exchange( other.m_header, nullptr ) )
		{
		}

		flexible_array& operator=( flexible_array&& other ) noexcept
		{
			if ( this != &other )
			{
				delete_all();
				if constexpr ( alloc_traits::propagate_on_container_move_assignment::value )
				{
					m_allocator = std::move( other.m_allocator );
				}
				else
				{
					DEBUG_ASSERT( m_allocator == other.m_allocator, "Containers incompatible for move assignment" );
				}
				m_header = std::exchange( other.m_header, nullptr );
			}
			return *this;
		}

		~flexible_array()
		{
			delete_all();
		}

		[[nodiscard]] allocator_type get_allocator() const noexcept
		{
			return m_allocator;
		}

		[[nodiscard]] size_type size() const noexcept
		{
			return m_header ? m_header->m_size : 0;
		}

		[[nodiscard]] T* data() noexcept
		{
			return m_header ? element_data() : nullptr;
		}

		[[nodiscard]] const T* data() const noexcept
		{
			return m_header ? element_data() : nullptr;
		}

		[[nodiscard]] reference operator[]( const Size index ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( index < size() );
			return data()[ index ];
		}

		[[nodiscard]] const_reference operator[]( const Size index ) const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( index < size() );
			return data()[ index ];
		}

		void swap( flexible_array& other ) noexcept
		{
			if ( this == &other )
			{
				return;
			}
			using std::swap;
			if constexpr ( alloc_traits::propagate_on_container_swap::value )
			{
				swap( m_allocator, other.m_allocator );
			}
			else
			{
				DEBUG_ASSERT( m_allocator == other.m_allocator, "Containers incompatible for swap" );
			}
			swap( m_header, other.m_header );
		}

		friend void swap( flexible_array& lhs, flexible_array& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

	private:
		[[nodiscard]] T* element_data() noexcept
		{
			return std::launder( reinterpret_cast<T*>( reinterpret_cast<std::byte*>( m_header ) + data_offset ) );
		}

		[[nodiscard]] const T* element_data() const noexcept
		{
			return std::launder(
				reinterpret_cast<const T*>( reinterpret_cast<const std::byte*>( m_header ) + data_offset ) );
		}

		[[nodiscard]] static constexpr std::size_t total_buffer_count( const Size size ) noexcept
		{
			const std::size_t total_bytes = data_offset + ( sizeof( T ) * size );
			return mclo::ceil_divide( total_bytes, sizeof( aligned_buffer ) );
		}

		void allocate_header( const Size size )
		{
			buffer_allocator alloc( m_allocator );
			aligned_buffer* const memory = buffer_alloc_traits::allocate( alloc, total_buffer_count( size ) );
			m_header = new ( memory ) header_type( size );
		}

		void delete_all() noexcept
		{
			if ( !m_header )
			{
				return;
			}
			std::destroy_n( element_data(), m_header->m_size );
			const std::size_t count = total_buffer_count( m_header->m_size );
			std::destroy_at( m_header );
			buffer_allocator alloc( m_allocator );
			buffer_alloc_traits::deallocate( alloc, reinterpret_cast<aligned_buffer*>( m_header ), count );
			m_header = nullptr;
		}

		static constexpr std::size_t data_offset = mclo::align_up( sizeof( header_type ), alignof( T ) );

		header_type* m_header = nullptr;
		MCLO_NO_UNIQUE_ADDRESS Allocator m_allocator;
	};
}
