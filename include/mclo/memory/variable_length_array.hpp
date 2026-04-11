#pragma once

#include <concepts>
#include <cstddef>

#include <mclo/numeric/align.hpp>
#include <mclo/debug/assert.hpp>

namespace mclo
{
	template <typename Size>
	struct variable_length_array_header
	{
		Size m_size = 0;
	};

	template <typename T, std::unsigned_integral Size = std::size_t>
	class variable_length_array
	{
		using header_type = variable_length_array_header<Size>;

	public:
		using value_type = T;
		using size_type = Size;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using const_pointer = const T*;
		using reference = T&;
		using const_reference = const T&;

		variable_length_array() = default;

		explicit variable_length_array( const Size size )
		{
			const std::size_t total_size = data_offset + ( sizeof( T ) * size );
			void* const memory = ::operator new( total_size, std::align_val_t{ data_alignment } );
			m_header = new ( memory ) header_type( size );
			std::uninitialized_default_construct_n( data(), size );
		}

		variable_length_array( const variable_length_array& other ) = delete;
		variable_length_array& operator=( const variable_length_array& other ) = delete;

		variable_length_array( variable_length_array&& other ) noexcept
			: m_header( std::exchange( other.m_header, nullptr ) )
		{
		}

		variable_length_array& operator=( variable_length_array&& other ) noexcept
		{
			if ( this != &other )
			{
				delete_all();
				m_header = std::exchange( other.m_header, nullptr );
			}
			return *this;
		}

		~variable_length_array()
		{
			delete_all();
		}

		[[nodiscard]] size_type size() const noexcept
		{
			return m_header ? m_header->m_size : 0;
		}

		[[nodiscard]] T* data() noexcept
		{
			if ( !m_header )
			{
				return nullptr;
			}
			return std::launder( reinterpret_cast<T*>( reinterpret_cast<std::byte*>( m_header ) + data_offset ) );
		}

		[[nodiscard]] const T* data() const noexcept
		{
			if ( !m_header )
			{
				return nullptr;
			}
			return std::launder(
				reinterpret_cast<const T*>( reinterpret_cast<const std::byte*>( m_header ) + data_offset ) );
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

	private:
		void delete_all() noexcept
		{
			if ( !m_header )
			{
				return;
			}
			std::destroy_n( data(), size() );
			std::destroy_at( m_header );
			::operator delete( m_header, std::align_val_t( data_alignment ) );
		}

		static constexpr std::size_t data_alignment = std::max( alignof( header_type ), alignof( T ) );
		static constexpr std::size_t data_offset = mclo::align_up( sizeof( header_type ), alignof( T ) );

		header_type* m_header = nullptr;
	};
}
