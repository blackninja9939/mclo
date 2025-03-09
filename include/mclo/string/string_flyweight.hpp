#pragma once

#include "mclo/memory/not_null.hpp"

#include <cstddef>
#include <cstring>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string_view>
#include <unordered_set>

namespace mclo
{
	namespace detail
	{
		/*
		* We want to provide a stable address, a single allocation, and know the size and char* from it
		* As such we use combine the allocation of a fixed size and align header, with a flexible size of chars
		* This is effectively C's flexible array members but well done manually cause C++ does not have that feature
		* Layout is: m_size, char0, char1, ..., char m_size - 1
		*/
		struct immutable_string_header
		{
			constexpr explicit immutable_string_header( const std::size_t size ) noexcept
				: m_size( size )
			{
			}

			std::size_t m_size = 0;

			[[nodiscard]] std::string_view view() const noexcept
			{
				return { reinterpret_cast<const char*>( this ) + sizeof( immutable_string_header ), m_size };
			}
		};

		class immutable_string_view
		{
			// Avoids branching in view() allowing us to keep this as a not_null
			static constexpr immutable_string_header empty_header{ 0 };

		public:
			constexpr immutable_string_view() noexcept
				: immutable_string_view( &empty_header )
			{
			}

			constexpr explicit immutable_string_view( mclo::not_null<const immutable_string_header*> str ) noexcept
				: m_string( str )
			{
			}

			[[nodiscard]] std::string_view view() const noexcept
			{
				return m_string->view();
			}

			[[nodiscard]] constexpr auto operator<=>( const immutable_string_view& other ) const noexcept = default;

		private:
			mclo::not_null<const immutable_string_header*> m_string;
		};

		class immutable_string
		{
		public:
			explicit immutable_string( const std::string_view str )
			{
				void* const ptr = ::operator new( str.size() + sizeof( immutable_string_header ),
												  std::align_val_t{ alignof( immutable_string_header ) } );
				static_assert( std::is_nothrow_constructible_v<immutable_string_header, std::string_view::size_type> );
				const immutable_string_header* const hdr =
					std::construct_at( static_cast<immutable_string_header*>( ptr ), str.size() );
				std::memcpy( static_cast<char*>( ptr ) + sizeof( immutable_string_header ), str.data(), str.size() );
				m_string.reset( hdr );
			}

			[[nodiscard]] operator immutable_string_view() const noexcept
			{
				return immutable_string_view( m_string.get() );
			}

			[[nodiscard]] std::string_view view() const noexcept
			{
				return m_string->view();
			}

		private:
			struct deleter
			{
				void operator()( const immutable_string_header* const ptr ) const noexcept
				{
					// No destructor calls required, just directly delete
					static_assert( std::is_trivially_destructible_v<immutable_string_header> );
					static_assert( std::is_trivially_destructible_v<char> );
					::operator delete( const_cast<void*>( static_cast<const void*>( ptr ) ),
									   std::align_val_t{ alignof( immutable_string_header ) } );
				}
			};
			std::unique_ptr<const immutable_string_header, deleter> m_string;
		};

		class string_flyweight_factory
		{
		public:
			using handle = immutable_string_view;

			handle insert( const std::string_view str )
			{
				if ( str.empty() )
				{
					return handle{};
				}

				{
					const std::shared_lock lock( m_mutex );
					const auto it = m_strings.find( str );
					if ( it != m_strings.end() )
					{
						return handle{ *it };
					}
				}

				immutable_string owned( str );

				const std::scoped_lock lock( m_mutex );

				return handle{ *m_strings.insert( std::move( owned ) ).first };
			}

			[[nodiscard]] static std::string_view get( const handle hdl ) noexcept
			{
				return hdl.view();
			}

		private:
			struct hasher
			{
				using is_transparent = void;
				[[nodiscard]] std::size_t operator()( const immutable_string& hdr ) const noexcept
				{
					return operator()( hdr.view() );
				}
				[[nodiscard]] std::size_t operator()( const std::string_view str ) const noexcept
				{
					return std::hash<std::string_view>()( str );
				}
			};
			struct equal_to
			{
				using is_transparent = void;
				[[nodiscard]] std::size_t operator()( const immutable_string& lhs,
													  const immutable_string& rhs ) const noexcept
				{
					return operator()( lhs.view(), rhs.view() );
				}
				[[nodiscard]] std::size_t operator()( const immutable_string& lhs,
													  const std::string_view rhs ) const noexcept
				{
					return operator()( lhs.view(), rhs );
				}
				[[nodiscard]] std::size_t operator()( const std::string_view lhs,
													  const immutable_string& rhs ) const noexcept
				{
					return operator()( lhs, rhs.view() );
				}
				[[nodiscard]] std::size_t operator()( const std::string_view lhs,
													  const std::string_view rhs ) const noexcept
				{
					return lhs == rhs;
				}
			};

			mutable std::shared_mutex m_mutex;
			std::unordered_set<immutable_string, hasher, equal_to> m_strings;
		};
	}

	template <typename Domain = void>
	class string_flyweight
	{
	public:
		constexpr string_flyweight() noexcept = default;

		explicit string_flyweight( const std::string_view str )
			: m_handle( factory().insert( str ) )
		{
		}

		string_flyweight& operator=( const std::string_view str )
		{
			m_handle = factory().insert( str );
			return *this;
		}

		[[nodiscard]] std::string_view get() const noexcept
		{
			return factory().get( m_handle );
		}

		[[nodiscard]] operator std::string_view() const noexcept
		{
			return get();
		}

		[[nodiscard]] constexpr auto operator<=>( const string_flyweight& other ) const noexcept = default;

	private:
		static detail::string_flyweight_factory& factory() noexcept
		{
			static detail::string_flyweight_factory instance;
			return instance;
		}

		detail::string_flyweight_factory::handle m_handle;
	};
}
