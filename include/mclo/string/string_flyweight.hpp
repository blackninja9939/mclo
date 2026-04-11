#pragma once

#include "mclo/numeric/math.hpp"

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
		 * todo(mc) long term it'd be good to use variable_length_array for this
		 */
		struct immutable_string_header
		{
			std::size_t m_size = 0;
		};

		template <typename CharT>
		[[nodiscard]] constexpr std::size_t calc_num_header_allocs( const std::size_t size ) noexcept
		{
			const std::size_t bytes = sizeof( immutable_string_header ) + ( size * sizeof( CharT ) );
			return mclo::ceil_divide( bytes, sizeof( immutable_string_header ) );
		}

		template <typename CharT, typename Allocator>
		struct string_deleter : Allocator
		{
			void operator()( const immutable_string_header* const ptr ) noexcept
			{
				// No destructor calls required, just directly deallocate
				static_assert( std::is_trivially_destructible_v<immutable_string_header> );
				static_assert( std::is_trivially_destructible_v<CharT> );
				static_cast<Allocator&>( *this ).deallocate( const_cast<immutable_string_header*>( ptr ),
															 calc_num_header_allocs<CharT>( ptr->m_size ) );
			}
		};

		template <typename CharT, typename Allocator>
		using allocated_string = std::unique_ptr<immutable_string_header, string_deleter<CharT, Allocator>>;

		template <typename ViewType, typename Allocator>
		[[nodiscard]] auto allocate_string( const ViewType str, Allocator allocator )
		{
			using char_type = typename ViewType::value_type;
			using size_type = typename ViewType::size_type;

			immutable_string_header* header = allocator.allocate( calc_num_header_allocs<char_type>( str.size() ) );

			static_assert( std::is_nothrow_constructible_v<immutable_string_header, size_type> );
			std::allocator_traits<Allocator>::construct( allocator, header, str.size() );
			const auto str_start = reinterpret_cast<std::byte*>( header ) + sizeof( immutable_string_header );
			std::memcpy( str_start, str.data(), str.size() * sizeof( char_type ) );

			using return_type = allocated_string<char_type, Allocator>;
			return return_type( header, typename return_type::deleter_type{ std::move( allocator ) } );
		}

		template <typename CharT, typename Traits = std::char_traits<CharT>, typename Allocator = std::allocator<CharT>>
		class string_flyweight_factory
		{
			using view = std::basic_string_view<CharT, Traits>;
			using header_allocator =
				typename std::allocator_traits<Allocator>::template rebind_alloc<immutable_string_header>;
			using string = allocated_string<CharT, header_allocator>;
			using map_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<string>;

		public:
			using handle = const immutable_string_header*;

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
						return it->get();
					}
				}

				string owned = allocate_string<view, header_allocator>( str, header_allocator( m_allocator ) );

				const std::scoped_lock lock( m_mutex );

				return m_strings.insert( std::move( owned ) ).first->get();
			}

			[[nodiscard]] static view get( const handle hdl ) noexcept
			{
				return hdl ? get( *hdl ) : view();
			}

		private:
			[[nodiscard]] static view get( const immutable_string_header& hdr ) noexcept
			{
				const auto str_start = reinterpret_cast<const std::byte*>( &hdr ) + sizeof( immutable_string_header );
				return { reinterpret_cast<const CharT*>( str_start ), hdr.m_size };
			}

			struct hasher
			{
				using is_transparent = void;

				[[nodiscard]] std::size_t operator()( const string& hdr ) const noexcept
				{
					return operator()( get( *hdr ) );
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
					return operator()( get( *lhs ), get( *rhs ) );
				}
				[[nodiscard]] bool operator()( const string& lhs, const view rhs ) const noexcept
				{
					return operator()( get( *lhs ), rhs );
				}
				[[nodiscard]] bool operator()( const view lhs, const string& rhs ) const noexcept
				{
					return operator()( lhs, get( *rhs ) );
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
