#pragma once

#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <ranges>

#include <mclo/debug/assert.hpp>
#include <mclo/numeric/align.hpp>
#include <mclo/numeric/math.hpp>
#include <mclo/platform/attributes.hpp>

namespace mclo
{
	/// @brief Header stored inline ahead of a @ref flexible_array's elements, holding the element count.
	/// @tparam Size The unsigned integer type used to store the count.
	template <std::unsigned_integral Size>
	struct flexible_array_header
	{
		Size m_size = 0;
	};

	/// @brief A fixed-size, heap-allocated array storing its size header and elements in a single allocation.
	/// @details Allocates one contiguous block holding a @ref flexible_array_header followed by the elements (a
	/// "flexible array member" layout), so the size and data share one allocation and are always contiguous in memory.
	/// The size is fixed at construction; there is no resizing or capacity beyond the element count. An empty array
	/// performs no allocation.
	/// @tparam T The element type.
	/// @tparam Size The unsigned integer type used for the size; smaller types shrink the header overhead.
	/// @tparam Allocator The allocator type. Fancy pointers are not supported.
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
		/// @brief The element type.
		using value_type = T;
		/// @brief The unsigned integer type used for the size.
		using size_type = Size;
		/// @brief Signed type for representing distances between elements.
		using difference_type = std::ptrdiff_t;
		/// @brief Pointer to an element.
		using pointer = T*;
		/// @brief Pointer to a const element.
		using const_pointer = const T*;
		/// @brief Reference to an element.
		using reference = T&;
		/// @brief Reference to a const element.
		using const_reference = const T&;
		/// @brief The allocator type.
		using allocator_type = Allocator;

		/// @brief Constructs an empty array with no allocation.
		flexible_array() = default;

		/// @brief Constructs an array of @p size default-constructed elements.
		/// @param size The number of elements to allocate and value-initialise.
		/// @param allocator The allocator to use.
		explicit flexible_array( const Size size, const Allocator& allocator = Allocator() )
			: m_allocator( allocator )
		{
			if ( size > 0 )
			{
				allocate_header( size );
				std::uninitialized_default_construct_n( element_data(), size );
			}
		}

		/// @brief Constructs an empty array using @p allocator, performing no allocation.
		explicit flexible_array( const Allocator& allocator ) noexcept
			: m_allocator( allocator )
		{
		}

		/// @brief Constructs from the elements in the iterator range [@p first, @p last).
		/// @param first Iterator to the first element to copy.
		/// @param last Sentinel marking the end of the range.
		/// @param allocator The allocator to use.
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

		/// @brief Constructs from the elements of @p range.
		/// @param range The forward range to copy elements from.
		/// @param allocator The allocator to use.
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

		/// @brief Copy constructor. Copies @p other's elements into a new allocation.
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

		/// @brief Copy assignment. Replaces contents with a copy of @p other's elements.
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

		/// @brief Move constructor. Takes over @p other's allocation, leaving it empty.
		flexible_array( flexible_array&& other ) noexcept
			: m_allocator( std::move( other.m_allocator ) )
			, m_header( std::exchange( other.m_header, nullptr ) )
		{
		}

		/// @brief Move assignment. Takes over @p other's allocation, leaving it empty.
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

		/// @brief Destructor. Destroys the elements and frees the allocation.
		~flexible_array()
		{
			delete_all();
		}

		/// @brief Returns a copy of the allocator.
		[[nodiscard]] allocator_type get_allocator() const noexcept
		{
			return m_allocator;
		}

		/// @brief Returns the number of elements.
		[[nodiscard]] size_type size() const noexcept
		{
			return m_header ? m_header->m_size : 0;
		}

		/// @brief Returns a pointer to the elements, or @c nullptr if empty.
		[[nodiscard]] T* data() noexcept
		{
			return m_header ? element_data() : nullptr;
		}

		/// @brief Returns a const pointer to the elements, or @c nullptr if empty.
		[[nodiscard]] const T* data() const noexcept
		{
			return m_header ? element_data() : nullptr;
		}

		/// @brief Returns a reference to the element at @p index.
		/// @pre @p index must be less than @ref size().
		[[nodiscard]] reference operator[]( const Size index ) noexcept
		{
			DEBUG_ASSERT( index < size() );
			return data()[ index ];
		}

		/// @brief Returns a const reference to the element at @p index.
		/// @pre @p index must be less than @ref size().
		[[nodiscard]] const_reference operator[]( const Size index ) const noexcept
		{
			DEBUG_ASSERT( index < size() );
			return data()[ index ];
		}

		/// @brief Swaps the contents with @p other.
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

		/// @brief Swaps the contents of two arrays.
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
