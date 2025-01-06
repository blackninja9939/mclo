#pragma once

#include "mclo/concepts/arithmetic.hpp"
#include "mclo/debug/assert.hpp"

#include <charconv>
#include <concepts>
#include <memory>
#include <string_view>

namespace mclo
{
	template <typename Derived>
	class string_builder_base
	{
	public:
		explicit string_builder_base( char* const buffer )
			: m_buffer( buffer )
		{
			DEBUG_ASSERT( buffer != nullptr, "Buffer must not be null" );
		}

		~string_builder_base()
		{
			static_cast<Derived&>( *this ).destroy_buffer( m_buffer );
		}

		string_builder_base( const string_builder_base& ) = delete;
		string_builder_base& operator=( const string_builder_base& ) = delete;
		string_builder_base( string_builder_base&& ) = delete;
		string_builder_base& operator=( string_builder_base&& ) = delete;

		void append( const char c )
		{
			grow_by_if_needed( 1 );
			m_buffer[ m_position++ ] = c;
		}

		template <std::convertible_to<std::string_view> StringView>
		void append( const StringView& str )
		{
			const std::string_view view( str );
			const std::size_t size = view.size();
			grow_by_if_needed( size );
			std::memcpy( write_start(), view.data(), size );
			m_position += size;
		}

		void append( const bool value )
		{
			append( value ? "true" : "false" );
		}

		template <arithmetic T>
		void append( const T value )
		{
			auto result = std::to_chars( write_start(), write_end(), value );
			if ( result.ec != std::errc{} )
			{
				static constexpr std::size_t max_integer_write_size = 20;
				grow_by_if_needed( max_integer_write_size );
				result = std::to_chars( write_start(), write_end(), value );
				DEBUG_ASSERT( result.ec == std::errc{}, "This write should definitely succeed" );
			}
			m_position = result.ptr - m_buffer;
		}

		std::string_view view() const noexcept
		{
			return std::string_view( m_buffer, m_position );
		}

		std::string to_string() const
		{
			return std::string( view() );
		}

		const char* c_str()
		{
			grow_by_if_needed( 1 );
			m_buffer[ m_position ] = '\0';
			return m_buffer;
		}

		void clear() noexcept
		{
			m_position = 0;
		}

	private:
		std::size_t capacity() const noexcept
		{
			return static_cast<const Derived&>( *this ).capacity();
		}

		char* write_start() const noexcept
		{
			return m_buffer + m_position;
		}
		char* write_end() const noexcept
		{
			return m_buffer + capacity();
		}

		void grow_by_if_needed( const std::size_t amount )
		{
			Derived& derived = static_cast<Derived&>( *this );

			const std::size_t new_size = m_position + amount;
			const std::size_t current_capacity = derived.capacity();
			if ( new_size <= current_capacity )
			{
				return;
			}

			const std::size_t new_capacity = std::max( current_capacity * 2, new_size );

			char* const new_buffer = derived.create_buffer( new_capacity );
			std::memcpy( new_buffer, m_buffer, m_position );

			derived.destroy_buffer( m_buffer );
			m_buffer = new_buffer;
			derived.set_capacity( new_capacity );
		}

		char* m_buffer = nullptr;
		std::size_t m_position = 0;
	};

	class string_builder : public string_builder_base<string_builder>
	{
		friend class string_builder_base<string_builder>;

	public:
		string_builder( const std::size_t inital_size = 1024 )
			: string_builder_base( create_buffer( inital_size ) )
			, m_capacity( inital_size )
		{
		}

	private:
		std::size_t capacity() const noexcept
		{
			return m_capacity;
		}
		void set_capacity( const std::size_t new_capacity ) noexcept
		{
			m_capacity = new_capacity;
		}

		char* create_buffer( const std::size_t new_capacity )
		{
			return new char[ new_capacity ];
		}
		void destroy_buffer( char* buffer )
		{
			delete[] buffer;
		}

		std::size_t m_capacity = 0;
	};

	template <std::size_t BufferSize>
	class fixed_string_builder : public string_builder_base<fixed_string_builder<BufferSize>>
	{
		friend class string_builder_base<fixed_string_builder<BufferSize>>;

	public:
		fixed_string_builder() noexcept
			: string_builder_base<fixed_string_builder<BufferSize>>( m_buffer )
		{
		}

	private:
		std::size_t capacity() const noexcept
		{
			return BufferSize;
		}
		void set_capacity( const std::size_t ) noexcept
		{
		}

		char* create_buffer( const std::size_t new_capacity )
		{
			PANIC( "Trying to grow fixed buffer", new_capacity );
		}
		void destroy_buffer( char* )
		{
		}

		char m_buffer[ BufferSize ];
	};
}
