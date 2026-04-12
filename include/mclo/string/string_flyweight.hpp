#pragma once

#include "mclo/memory/flexible_array.hpp"
#include "mclo/platform/attributes.hpp"

#include <mutex>
#include <shared_mutex>
#include <string_view>
#include <unordered_set>

namespace mclo
{
	namespace detail
	{
		template <typename CharT, typename Traits = std::char_traits<CharT>, typename Allocator = std::allocator<CharT>>
		class string_flyweight_factory
		{
			using view = std::basic_string_view<CharT, Traits>;
			using string = mclo::flexible_array<CharT, std::size_t, Allocator>;
			using map_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<string>;

		public:
			using handle = const string*;

			string_flyweight_factory() = default;

			explicit string_flyweight_factory( const Allocator& allocator )
				: m_allocator( allocator )
			{
			}

			handle insert( const view str )
			{
				if ( str.empty() ) [[unlikely]]
				{
					return nullptr;
				}

				{
					const std::shared_lock lock( m_mutex );
					auto it = m_strings.find( str );
					if ( it != m_strings.end() ) [[likely]]
					{
						return &*it;
					}
				}

				string owned( str, m_allocator );

				const std::scoped_lock lock( m_mutex );

				return &*m_strings.insert( std::move( owned ) ).first;
			}

			[[nodiscard]] static view get( const handle hdl ) noexcept
			{
				return hdl ? view( hdl->data(), hdl->size() ) : view();
			}

		private:
			[[nodiscard]] static view view_of( const string& s ) noexcept
			{
				return { s.data(), s.size() };
			}

			struct hasher
			{
				using is_transparent = void;

				[[nodiscard]] std::size_t operator()( const string& s ) const noexcept
				{
					return operator()( view_of( s ) );
				}
				[[nodiscard]] std::size_t operator()( const view str ) const noexcept
				{
					return std::hash<view>()( str );
				}
			};

			struct equal_to
			{
				using is_transparent = void;
				[[nodiscard]] bool operator()( const string& lhs, const string& rhs ) const noexcept
				{
					return view_of( lhs ) == view_of( rhs );
				}
				[[nodiscard]] bool operator()( const string& lhs, const view rhs ) const noexcept
				{
					return view_of( lhs ) == rhs;
				}
				[[nodiscard]] bool operator()( const view lhs, const string& rhs ) const noexcept
				{
					return lhs == view_of( rhs );
				}
				[[nodiscard]] bool operator()( const view lhs, const view rhs ) const noexcept
				{
					return lhs == rhs;
				}
			};

			mutable std::shared_mutex m_mutex;
			MCLO_NO_UNIQUE_ADDRESS Allocator m_allocator;
			std::unordered_set<string, hasher, equal_to, map_allocator> m_strings{ map_allocator( m_allocator ) };
		};
	}

	/// @brief A flyweight string that stores a single copy of each unique string in a shared pool.
	/// @details This class is useful when you have a large number of strings that are mostly the same, and you want to
	/// reduce memory usage by only allocating a single copy of each unique string. This class is thread-safe using a
	/// shared mutex for insertion. Reading the string requires no locking as its a unique allocation, it is a single
	/// pointer dereference.
	/// @tparam Domain The domain of the flyweight, used to separate different shared string pools. Different domains
	/// have different mutexes so will not block each other.
	/// @tparam CharT The type of character for the underlying strings
	/// @tparam Traits The traits for the underlying strings
	/// @tparam Allocator The allocator for the underlying strings
	template <typename Domain,
			  typename CharT,
			  typename Traits = std::char_traits<CharT>,
			  typename Allocator = std::allocator<CharT>>
	class basic_string_flyweight
	{
		using factory_t = detail::string_flyweight_factory<CharT, Traits, Allocator>;

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
			return factory_t::get( m_handle );
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

		factory_t::handle m_handle{};
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
