#pragma once

#include "mclo/allocator/arena_allocator.hpp"
#include "mclo/memory/byte_literals.hpp"
#include "mclo/memory/flexible_array.hpp"

#include <mutex>
#include <shared_mutex>
#include <string_view>
#include <unordered_set>

namespace mclo
{
	namespace detail
	{
		template <typename Domain, typename CharT, typename Traits = std::char_traits<CharT>>
		class string_flyweight_factory
		{
			using view = std::basic_string_view<CharT, Traits>;
			using arena_alloc = mclo::arena_allocator<CharT>;
			using string = mclo::flexible_array<CharT, std::size_t, arena_alloc>;

		public:
			using handle = const string*;

			[[nodiscard]] static string_flyweight_factory& instance() noexcept
			{
				static string_flyweight_factory instance;
				return instance;
			}

			handle insert( const view str )
			{
				if ( str.empty() ) [[unlikely]]
				{
					return nullptr;
				}

				{
					const std::shared_lock lock( m_mutex );
					const auto it = m_strings.find( str );
					if ( it != m_strings.end() ) [[likely]]
					{
						return &*it;
					}
				}

				const std::scoped_lock lock( m_mutex );

				// Re-check under the unique lock: another thread may have inserted the same string between releasing
				// the shared lock and acquiring this one. The arena allocates under this lock and cannot reclaim
				// individual allocations, so we must only allocate once we are certain the string is new.
				const auto it = m_strings.find( str );
				if ( it != m_strings.end() )
				{
					return &*it;
				}

				string owned( str, m_arena );
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

			string_flyweight_factory()
			{
				m_strings.reserve( 1024 );
			}

			/*
			 * todo(mc) if read lock contention becomes heavy we can optimize this more using sharding, we store N
			 * unordered sets and mutexes, and hash the string to determine which shard to use.
			 * We'd also then want to avoid hashing repeatedly to determine shard and to find the string and potentially
			 * use a different set implementation that is optimized for insertions and doesn't require deletions.
			 * We'd also likely want one arena still to avoid wasting memory, so we'd need a lock free arena allocator.
			 */

			mutable std::shared_mutex m_mutex;
			memory_arena m_arena{ 4_KiB };
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
	/// @tparam CharT The type of character for the underlying strings
	/// @tparam Traits The traits for the underlying strings
	template <typename Domain, typename CharT, typename Traits = std::char_traits<CharT>>
	class basic_string_flyweight
	{
		using factory_t = detail::string_flyweight_factory<Domain, CharT, Traits>;

	public:
		/// @brief The string view type used to read the interned string.
		using view = std::basic_string_view<CharT, Traits>;

		/// @brief Constructs an empty flyweight that refers to no string.
		constexpr basic_string_flyweight() noexcept = default;

		/// @brief Interns @p str in the shared pool and refers to the stored copy.
		/// @param str The string to intern.
		explicit basic_string_flyweight( const view str )
			: m_handle( factory_t::instance().insert( str ) )
		{
		}

		/// @brief Interns @p str in the shared pool and refers to the stored copy.
		/// @param str The string to intern.
		basic_string_flyweight& operator=( const view str )
		{
			m_handle = factory_t::instance().insert( str );
			return *this;
		}

		/// @brief Returns a view of the referenced string, or an empty view if none.
		[[nodiscard]] view get() const noexcept
		{
			return factory_t::get( m_handle );
		}

		/// @brief Converts to a view of the referenced string.
		[[nodiscard]] operator view() const noexcept
		{
			return get();
		}

		/// @brief Compares two flyweights. Cheap, as it compares interned handles, not string contents.
		[[nodiscard]] constexpr auto operator<=>( const basic_string_flyweight& other ) const noexcept = default;

		/// @brief Swaps the referenced strings of two flyweights.
		void swap( basic_string_flyweight& other ) noexcept
		{
			using std::swap;
			swap( m_handle, other.m_handle );
		}

		/// @brief Swaps the referenced strings of two flyweights.
		friend void swap( basic_string_flyweight& lhs, basic_string_flyweight& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

	private:
		factory_t::handle m_handle{};
	};

	/// @brief A @ref basic_string_flyweight of @c char.
	template <typename Domain = void>
	using string_flyweight = basic_string_flyweight<Domain, char>;

	/// @brief A @ref basic_string_flyweight of @c wchar_t.
	template <typename Domain = void>
	using wstring_flyweight = basic_string_flyweight<Domain, wchar_t>;

#ifdef __cpp_lib_char8_t
	/// @brief A @ref basic_string_flyweight of @c char8_t.
	template <typename Domain = void>
	using u8string_flyweight = basic_string_flyweight<Domain, char8_t>;
#endif

	/// @brief A @ref basic_string_flyweight of @c char16_t.
	template <typename Domain = void>
	using u16string_flyweight = basic_string_flyweight<Domain, char16_t>;

	/// @brief A @ref basic_string_flyweight of @c char32_t.
	template <typename Domain = void>
	using u32string_flyweight = basic_string_flyweight<Domain, char32_t>;
}
