#pragma once

#include "mclo/memory/not_null.hpp"
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
		 */
		struct immutable_string_header
		{
			std::size_t m_size = 0;

			static constexpr void acquire() noexcept
			{
			}
			[[nodiscard]] static constexpr bool release() noexcept
			{
				return false;
			}
		};

		struct ref_counted_immutable_string_header
		{
			std::size_t m_size = 0;
			std::atomic_size_t m_references{ 1 };

			void acquire() noexcept
			{
				m_references.fetch_add( 1, std::memory_order_relaxed );
			}
			[[nodiscard]] bool release() noexcept
			{
				const std::size_t last_count = m_references.fetch_sub( 1, std::memory_order_acq_rel );
				return last_count == 1; // We were last owner so caller should delete
			}
		};

		template <typename CharT, typename Traits, typename Header>
		[[nodiscard]] std::basic_string_view<CharT, Traits> view_header( const Header& header ) noexcept
		{
			const auto str_start = reinterpret_cast<const std::byte*>( &header ) + sizeof( Header );
			return { reinterpret_cast<const CharT*>( str_start ), header.m_size };
		}

		template <typename Header, typename CharT>
		[[nodiscard]] constexpr std::size_t calc_num_header_allocs( const std::size_t size ) noexcept
		{
			const std::size_t bytes = sizeof( Header ) + ( size * sizeof( CharT ) );
			return mclo::ceil_divide( bytes, sizeof( Header ) );
		}

		template <typename Header, typename CharT, typename Allocator>
		struct string_deleter : Allocator
		{
			void operator()( const Header* const ptr ) noexcept
			{
				// No destructor calls required, just directly deallocate
				static_assert( std::is_trivially_destructible_v<Header> );
				static_assert( std::is_trivially_destructible_v<char> );
				static_cast<Allocator&>( *this ).deallocate( const_cast<Header*>( ptr ),
															 calc_num_header_allocs<Header, CharT>( ptr->m_size ) );
			}
		};

		template <typename Header, typename CharT, typename Allocator>
		using allocated_string = std::unique_ptr<Header, string_deleter<Header, CharT, Allocator>>;

		template <typename ViewType, typename Header, typename Allocator>
		[[nodiscard]] auto allocate_string( const ViewType str, Allocator allocator )
		{
			using char_type = typename ViewType::value_type;
			using size_type = typename ViewType::size_type;

			Header* header = allocator.allocate( calc_num_header_allocs<Header, char_type>( str.size() ) );

			static_assert( std::is_nothrow_constructible_v<Header, size_type> );
			header = std::construct_at( header, str.size() );
			const auto str_start = reinterpret_cast<std::byte*>( header ) + sizeof( Header );
			std::memcpy( str_start, str.data(), str.size() * sizeof( char_type ) );

			using return_type = allocated_string<Header, char_type, Allocator>;
			return return_type( header, typename return_type::deleter_type{ std::move( allocator ) } );
		}

		template <typename Header,
				  typename CharT,
				  typename Traits = std::char_traits<CharT>,
				  typename Allocator = std::allocator<CharT>>
		class string_flyweight_factory
		{
			using view = std::basic_string_view<CharT, Traits>;
			using header_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Header>;
			using string = allocated_string<Header, CharT, header_allocator>;

		public:
			using handle = const Header*;

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
						handle hdl = it->get();
						hdl->acquire();
						return hdl;
					}
				}

				string owned = allocate_string<view, Header, header_allocator>( str, header_allocator( m_allocator ) );

				const std::scoped_lock lock( m_mutex );

				return m_strings.insert( std::move( owned ) ).first->get();
			}

			void release( const handle hdl )
			{
				if ( !hdl )
				{
					return;
				}
				const bool should_erase = const_cast<Header*>( hdl )->release();
				if ( should_erase ) [[unlikely]]
				{
					// todo(mc) is this thread safe?
					const std::scoped_lock lock( m_mutex );
					m_strings.erase( m_strings.find( get( *hdl ) ) );
				}
			}

			[[nodiscard]] static view get( const handle hdl ) noexcept
			{
				return hdl ? get( *hdl ) : view();
			}

		private:
			[[nodiscard]] static view get( const Header& hdr ) noexcept
			{
				return view_header<CharT, Traits>( hdr );
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
				[[nodiscard]] std::size_t operator()( const string& lhs, const string& rhs ) const noexcept
				{
					return operator()( get( *lhs ), get( *rhs ) );
				}
				[[nodiscard]] std::size_t operator()( const string& lhs, const view rhs ) const noexcept
				{
					return operator()( get( *lhs ), rhs );
				}
				[[nodiscard]] std::size_t operator()( const view lhs, const string& rhs ) const noexcept
				{
					return operator()( lhs, get( *rhs ) );
				}
				[[nodiscard]] std::size_t operator()( const view lhs, const view rhs ) const noexcept
				{
					return lhs == rhs;
				}
			};

			mutable std::shared_mutex m_mutex;
			MCLO_NO_UNIQUE_ADDRESS Allocator m_allocator;
			std::unordered_set<string, hasher, equal_to> m_strings{
				static_cast<typename std::allocator_traits<Allocator>::template rebind_alloc<string>>( m_allocator ) };
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
	template <typename Domain,
			  typename Header,
			  typename CharT,
			  typename Traits = std::char_traits<CharT>,
			  typename Allocator = std::allocator<CharT>>
	class basic_string_flyweight
	{
		using factory_t = detail::string_flyweight_factory<Header, CharT, Traits, Allocator>;

	public:
		using view = std::basic_string_view<CharT, Traits>;

		constexpr basic_string_flyweight() noexcept = default;

		explicit basic_string_flyweight( const view str )
			: m_handle( factory().insert( str ) )
		{
		}

		~basic_string_flyweight()
		{
			factory().release( m_handle );
		}

		basic_string_flyweight& operator=( const view str )
		{
			factory_t& fac = factory();
			fac.release( m_handle );
			m_handle = fac.insert( str );
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

	template <typename Domain,
			  typename CharT,
			  typename Traits = std::char_traits<CharT>,
			  typename Allocator = std::allocator<CharT>>
	using immutable_string_flyweight =
		basic_string_flyweight<Domain, detail::immutable_string_header, CharT, Traits, Allocator>;

	template <typename Domain,
			  typename CharT,
			  typename Traits = std::char_traits<CharT>,
			  typename Allocator = std::allocator<CharT>>
	using ref_counted_string_flyweight =
		basic_string_flyweight<Domain, detail::ref_counted_immutable_string_header, CharT, Traits, Allocator>;

	template <typename Domain = void>
	using string_flyweight = immutable_string_flyweight<Domain, char>;

	template <typename Domain = void>
	using wstring_flyweight = immutable_string_flyweight<Domain, wchar_t>;

#ifdef __cpp_lib_char8_t
	template <typename Domain = void>
	using u8string_flyweight = immutable_string_flyweight<Domain, char8_t>;
#endif

	template <typename Domain = void>
	using u16string_flyweight = immutable_string_flyweight<Domain, char16_t>;

	template <typename Domain = void>
	using u32string_flyweight = immutable_string_flyweight<Domain, char32_t>;
}
