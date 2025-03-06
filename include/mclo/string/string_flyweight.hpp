#pragma once

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
		struct string_header
		{
			constexpr explicit string_header( const std::size_t size ) noexcept
				: m_size( size )
			{
			}

			std::size_t m_size = 0;

			[[nodiscard]] std::string_view string() const noexcept
			{
				return { reinterpret_cast<const char*>( this ) + sizeof( string_header ), m_size };
			}
		};

		struct string_deleter
		{
			void operator()( const string_header* const ptr ) const noexcept
			{
				static_assert( std::is_trivially_destructible_v<string_header> );
				static_assert( std::is_trivially_destructible_v<char> );
				::operator delete( const_cast<void*>( static_cast<const void*>( ptr ) ),
								   std::align_val_t{ alignof( string_header ) } );
			}
		};

		using fixed_heap_string = std::unique_ptr<const string_header, string_deleter>;

		[[nodiscard]] static fixed_heap_string make_fixed_heap_string( const std::string_view str )
		{
			void* ptr =
				::operator new( str.size() + sizeof( string_header ), std::align_val_t{ alignof( string_header ) } );
			const string_header* const hdr = std::construct_at( static_cast<string_header*>( ptr ), str.size() );
			std::memcpy( static_cast<char*>( ptr ) + sizeof( string_header ), str.data(), str.size() );
			return fixed_heap_string( hdr );
		}

		class string_flyweight_factory
		{
			// Avoids branching in get
			static constexpr string_header null_header{ 0 };

		public:
			class handle
			{
			public:
				friend string_flyweight_factory;

				constexpr handle() noexcept = default;

				[[nodiscard]] constexpr auto operator<=>( const handle& other ) const noexcept = default;

			private:
				constexpr handle( const string_header* ptr ) noexcept
					: m_ptr( ptr )
				{
				}

				const string_header* m_ptr = &null_header;
			};

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
						return handle{ it->get() };
					}
				}

				auto owned = make_fixed_heap_string( str );

				const std::scoped_lock lock( m_mutex );

				return handle{ m_strings.insert( std::move( owned ) ).first->get() };
			}

			[[nodiscard]] static std::string_view get( const handle hdl ) noexcept
			{
				return hdl.m_ptr->string();
			}

		private:
			struct hasher
			{
				using is_transparent = void;
				[[nodiscard]] std::size_t operator()( const fixed_heap_string& hdr ) const noexcept
				{
					return operator()( hdr->string() );
				}
				[[nodiscard]] std::size_t operator()( const std::string_view str ) const noexcept
				{
					return std::hash<std::string_view>()( str );
				}
			};
			struct equal_to
			{
				using is_transparent = void;
				[[nodiscard]] std::size_t operator()( const fixed_heap_string& lhs,
													  const fixed_heap_string& rhs ) const noexcept
				{
					return operator()( lhs->string(), rhs->string() );
				}
				[[nodiscard]] std::size_t operator()( const fixed_heap_string& lhs,
													  const std::string_view rhs ) const noexcept
				{
					return operator()( lhs->string(), rhs );
				}
				[[nodiscard]] std::size_t operator()( const std::string_view lhs,
													  const fixed_heap_string& rhs ) const noexcept
				{
					return operator()( lhs, rhs->string() );
				}
				[[nodiscard]] std::size_t operator()( const std::string_view lhs,
													  const std::string_view rhs ) const noexcept
				{
					return lhs == rhs;
				}
			};

			mutable std::shared_mutex m_mutex;
			std::unordered_set<fixed_heap_string, hasher, equal_to> m_strings;
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

		detail::string_flyweight_factory::handle m_handle{};
	};
}
