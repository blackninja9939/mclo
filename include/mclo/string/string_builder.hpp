#pragma once

#include "mclo/concepts/arithmetic.hpp"
#include "mclo/debug/assert.hpp"
#include "mclo/preprocessor/platform.hpp"

#include <charconv>
#include <concepts>
#include <memory>
#include <string_view>

namespace mclo
{
	// todo(mc) this API lacks the ability to take variadics which is a bit annoying, additionally it cannot resize in one go by such variadic
	// probably best to nuke this and make a join_string overload that takes views, chars, bools and numbers and converts to a buffer + resizes in one go
	template <typename Buffer>
	class string_builder_base
	{
	public:
		string_builder_base() noexcept( std::is_nothrow_default_constructible_v<Buffer> )
			requires std::constructible_from<Buffer>
		= default;

		template <typename... Args>
		explicit( !std::convertible_to<Buffer, Args...> )
			string_builder_base( Args&&... args ) noexcept( std::is_nothrow_constructible_v<Buffer, Args...> )
			: m_buffer( std::forward<Args>( args )... )
		{
		}

		void append( const char c )
		{
			grow_by_if_needed( 1 );
			buffer()[ m_position++ ] = c;
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
			m_position = result.ptr - buffer();
		}

		std::string_view view() const noexcept
		{
			return std::string_view( buffer(), m_position );
		}

		std::string to_string() const
		{
			return std::string( view() );
		}

		const char* c_str()
		{
			grow_by_if_needed( 1 );
			char* const buff = buffer();
			buff[ m_position ] = '\0';
			return buff;
		}

		void clear() noexcept
		{
			m_position = 0;
		}

	private:
		std::size_t capacity() const noexcept
		{
			return m_buffer.capacity();
		}

		const char* buffer() const noexcept
		{
			return m_buffer.get();
		}
		char* buffer() noexcept
		{
			return m_buffer.get();
		}

		char* write_start() noexcept
		{
			return buffer() + m_position;
		}
		char* write_end() noexcept
		{
			return buffer() + capacity();
		}

		void grow_by_if_needed( const std::size_t amount )
		{
			const std::size_t new_size = m_position + amount;
			const std::size_t current_capacity = capacity();
			if ( new_size <= current_capacity )
			{
				return;
			}

			const std::size_t new_capacity = std::max( current_capacity * 2, new_size );
			m_buffer.resize( m_position, new_capacity );
		}

		Buffer m_buffer{};
		std::size_t m_position = 0;
	};

	namespace detail
	{
		struct dynamic_string_buffer
		{
			dynamic_string_buffer( const std::size_t capacity = 1024 )
				: m_buffer( std::make_unique<char[]>( capacity ) )
				, m_capacity( capacity )
			{
			}

			char* get() const noexcept
			{
				return m_buffer.get();
			}

			std::size_t capacity() const noexcept
			{
				return m_capacity;
			}

			void resize( const std::size_t size, const std::size_t new_capacity )
			{
				auto new_buffer = std::make_unique<char[]>( new_capacity );
				std::memcpy( new_buffer.get(), get(), size );
				m_buffer = std::move( new_buffer );
				m_capacity = new_capacity;
			}

			std::unique_ptr<char[]> m_buffer;
			std::size_t m_capacity;
		};

		template <std::size_t Capacity>
		struct fixed_string_buffer
		{
			MCLO_MSVC_PUSH_AND_DISABLE_WARNINGS( 26495 )
			fixed_string_buffer() = default;
			MCLO_MSVC_POP_WARNINGS

			const char* get() const noexcept
			{
				return m_buffer;
			}
			char* get() noexcept
			{
				return m_buffer;
			}

			static constexpr std::size_t capacity() noexcept
			{
				return Capacity;
			}

			[[noreturn]] void resize( const std::size_t, const std::size_t new_capacity )
			{
				PANIC( "Trying to grow fixed size string builder buffer", new_capacity, Capacity );
			}

			char m_buffer[ Capacity ];
		};
	}

	using string_builder = string_builder_base<detail::dynamic_string_buffer>;

	template <std::size_t Capacity>
	using fixed_string_builder = string_builder_base<detail::fixed_string_buffer<Capacity>>;
}
