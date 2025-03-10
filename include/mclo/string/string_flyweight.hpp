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

			template <typename CharT, typename Traits>
			[[nodiscard]] std::basic_string_view<CharT, Traits> view() const noexcept
			{
				const auto str_start = reinterpret_cast<const std::byte*>( this ) + sizeof( immutable_string_header );
				return { reinterpret_cast<const CharT*>( str_start ), m_size };
			}
		};

		template <typename CharT, typename Traits = std::char_traits<CharT>>
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

			[[nodiscard]] std::basic_string_view<CharT, Traits> view() const noexcept
			{
				return m_string->view<CharT, Traits>();
			}

			[[nodiscard]] constexpr auto operator<=>( const immutable_string_view& other ) const noexcept = default;

			void swap( immutable_string_view& other ) noexcept
			{
				using std::swap;
				swap( m_string, other.m_string );
			}

			friend void swap( immutable_string_view& lhs, immutable_string_view& rhs ) noexcept
			{
				lhs.swap( rhs );
			}

		private:
			mclo::not_null<const immutable_string_header*> m_string;
		};

		template <typename CharT, typename Traits = std::char_traits<CharT>>
		class immutable_string
		{
			using view_type = std::basic_string_view<CharT, Traits>;
			static constexpr std::align_val_t alloc_align{
				std::max( alignof( immutable_string_header ), alignof( CharT ) ) };

		public:
			explicit immutable_string( const view_type str )
			{
				const std::size_t bytes = sizeof( immutable_string_header ) + ( str.size() * sizeof( CharT ) );
				void* const ptr = ::operator new( bytes, alloc_align );
				static_assert( std::is_nothrow_constructible_v<immutable_string_header, view_type::size_type> );
				const immutable_string_header* const hdr =
					std::construct_at( static_cast<immutable_string_header*>( ptr ), str.size() );
				const auto str_start = static_cast<std::byte*>( ptr ) + sizeof( immutable_string_header );
				std::memcpy( str_start, str.data(), str.size() * sizeof( CharT ) );
				m_string.reset( hdr );
			}

			[[nodiscard]] operator immutable_string_view<CharT, Traits>() const noexcept
			{
				return immutable_string_view<CharT, Traits>( m_string.get() );
			}

			[[nodiscard]] view_type view() const noexcept
			{
				return m_string->view<CharT, Traits>();
			}

			void swap( immutable_string& other ) noexcept
			{
				using std::swap;
				swap( m_string, other.m_string );
			}

			friend void swap( immutable_string& lhs, immutable_string& rhs ) noexcept
			{
				lhs.swap( rhs );
			}

		private:
			struct deleter
			{
				void operator()( const immutable_string_header* const ptr ) const noexcept
				{
					// No destructor calls required, just directly delete
					static_assert( std::is_trivially_destructible_v<immutable_string_header> );
					static_assert( std::is_trivially_destructible_v<char> );
					::operator delete( const_cast<void*>( static_cast<const void*>( ptr ) ), alloc_align );
				}
			};
			std::unique_ptr<const immutable_string_header, deleter> m_string;
		};

		template <typename CharT, typename Traits = std::char_traits<CharT>>
		class string_flyweight_factory
		{
			using string = immutable_string<CharT, Traits>;
			using view = std::basic_string_view<CharT, Traits>;

		public:
			using handle = immutable_string_view<CharT, Traits>;

			handle insert( const view str )
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

				string owned( str );

				const std::scoped_lock lock( m_mutex );

				return handle{ *m_strings.insert( std::move( owned ) ).first };
			}

		private:
			struct hasher
			{
				using is_transparent = void;
				[[nodiscard]] std::size_t operator()( const string& hdr ) const noexcept
				{
					return operator()( hdr.view() );
				}
				[[nodiscard]] std::size_t operator()( const view str ) const noexcept
				{
					return std::hash<view>()( str );
				}
			};

			struct equal_to
			{
				using is_transparent = void;
				[[nodiscard]] std::size_t operator()( const string& lhs, const string& rhs ) const noexcept
				{
					return operator()( lhs.view(), rhs.view() );
				}
				[[nodiscard]] std::size_t operator()( const string& lhs, const view rhs ) const noexcept
				{
					return operator()( lhs.view(), rhs );
				}
				[[nodiscard]] std::size_t operator()( const view lhs, const string& rhs ) const noexcept
				{
					return operator()( lhs, rhs.view() );
				}
				[[nodiscard]] std::size_t operator()( const view lhs, const view rhs ) const noexcept
				{
					return lhs == rhs;
				}
			};

			mutable std::shared_mutex m_mutex;
			std::unordered_set<string, hasher, equal_to> m_strings;
		};
	}

	/// @brief A flyweight string that stores a single copy of each unique string in a shared pool.
	/// @details This class is useful when you have a large number of strings that are mostly the same, and you want to
	/// reduce memory usage by only allocating a single copy of each unique string. This class is thread-safe using a
	/// shared mutex for insertion. Reading the string requires no locking as its a unique allocation, it is a single
	/// pointer dereference.
	/// @tparam Domain The domain of the flyweight, used to separate different shared string pools. Different domains
	/// have different mutexes so will not block each other.
	template <typename Domain, typename CharT, typename Traits = std::char_traits<CharT>>
	class basic_string_flyweight
	{
		using factory_t = detail::string_flyweight_factory<CharT, Traits>;

	public:
		using view = std::basic_string_view<CharT, Traits>;

		constexpr basic_string_flyweight() noexcept = default;

		explicit basic_string_flyweight( const view str )
			: m_handle( factory().insert( str ) )
		{
		}

		basic_string_flyweight& operator=( const view str )
		{
			m_handle = factory().insert( str );
			return *this;
		}

		[[nodiscard]] view get() const noexcept
		{
			return m_handle.view();
		}

		[[nodiscard]] operator view() const noexcept
		{
			return get();
		}

		[[nodiscard]] constexpr auto operator<=>( const basic_string_flyweight& other ) const noexcept = default;

		void swap( basic_string_flyweight& other ) noexcept
		{
			using std::swap;
			swap( m_handle, other.m_handle );
		}

		friend void swap( basic_string_flyweight& lhs, basic_string_flyweight& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

	private:
		static factory_t& factory() noexcept
		{
			static factory_t instance;
			return instance;
		}

		factory_t::handle m_handle;
	};

	template <typename Domain = void>
	using string_flyweight = basic_string_flyweight<Domain, char>;

	template <typename Domain = void>
	using wstring_flyweight = basic_string_flyweight<Domain, wchar_t>;

#ifdef __cpp_lib_char8_t
	template <typename Domain = void>
	using u8string_flyweight = basic_string_flyweight<Domain, char8_t>;
#endif

	template <typename Domain = void>
	using u16string_flyweight = basic_string_flyweight<Domain, char16_t>;

	template <typename Domain = void>
	using u32string_flyweight = basic_string_flyweight<Domain, char32_t>;
}
