#pragma once

#include "small_optional_integer.hpp"

#include <atomic>
#include <cinttypes>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string_view>
#include <unordered_map>

namespace mclo
{
	namespace detail
	{
		inline std::atomic_flag domain_used = ATOMIC_FLAG_INIT;
	}

	class heap_string
	{
	public:
		explicit heap_string( const std::string_view str )
			: m_string( std::make_unique<char[]>( str.size() + 1 ) )
			, m_size( str.size() )
		{
			std::strncpy( m_string.get(), str.data(), str.size() );
		}

		operator std::string_view() const noexcept
		{
			return { m_string.get(), m_size };
		}

	private:
		std::unique_ptr<char[]> m_string;
		std::size_t m_size;
	};

	template <typename Domain, typename IndexType>
	class string_table;

	template <typename Domain, typename IndexType>
	class string_handle : private mclo::small_optional_integer<IndexType>
	{
		using base = mclo::small_optional_integer<IndexType>;
	public:
		static_assert( std::is_unsigned_v<IndexType>, "IndexType must be unsigned" );
		friend class string_table<Domain, IndexType>;

		constexpr bool is_valid() const noexcept
		{
			return has_value();
		}

		[[nodsicard]] constexpr bool operator==( const string_handle other) const noexcept
		{
			return static_cast<const base&>( *this ) == static_cast<const base&>( other );
		}
		[[nodsicard]] constexpr bool operator!=( const string_handle other ) const noexcept
		{
			return static_cast<const base&>( *this ) != static_cast<const base&>( other );
		}
		[[nodsicard]] constexpr bool operator<( const string_handle other ) const noexcept
		{
			return static_cast<const base&>( *this ) < static_cast<const base&>( other );
		}
		[[nodsicard]] constexpr bool operator<=( const string_handle other ) const noexcept
		{
			return static_cast<const base&>( *this ) <= static_cast<const base&>( other );
		}
		[[nodsicard]] constexpr bool operator>( const string_handle other ) const noexcept
		{
			return static_cast<const base&>( *this ) > static_cast<const base&>( other );
		}
		[[nodsicard]] constexpr bool operator>=( const string_handle other ) const noexcept
		{
			return static_cast<const base&>( *this ) >= static_cast<const base&>( other );
		}

	private:
		using base::base;
		using base::value;
		using base::max_value;
	};

	template <typename Domain, typename IndexType = std::uint16_t>
	class string_table
	{
	public:
		static_assert( std::is_unsigned_v<IndexType>, "IndexType must be unsigned" );
		using handle = string_handle<Domain, IndexType>;

		string_table()
		{
			const bool was_used = detail::domain_used.test_and_set();
			assert( !was_used );
		}
		~string_table()
		{
			detail::domain_used.clear();
		}

		handle lookup_handle( const std::string_view str ) const noexcept
		{
			if ( str.empty() )
			{
				return {};
			}

			const std::shared_lock lock( m_mutex );
			const auto it = m_string_to_handle.find( str );
			if ( it == m_string_to_handle.end() )
			{
				return {};
			}

			return it->second;
		}

		std::string_view lookup_string( const handle hdl ) const noexcept
		{
			if ( !hdl.is_valid() )
			{
				return {};
			}

			const std::shared_lock lock( m_mutex );
			return m_handle_to_string[ hdl.value() ];
		}

		handle insert( const std::string_view str )
		{
			if ( str.empty() )
			{
				return {};
			}

			if ( handle ret = lookup_handle( str ); ret.is_valid() )
			{
				return ret;
			}

			heap_string owned{ str };

			const std::scoped_lock lock( m_mutex );

			const auto [ it, inserted ] = m_string_to_handle.emplace( owned, handle{} );

			if ( inserted )
			{
				const std::size_t next_index = m_handle_to_string.size();
				assert( next_index <= handle::max_value );
				it->second = handle{ static_cast<IndexType>( next_index ) };
				m_handle_to_string.push_back( std::move( owned ) );
			}

			return it->second;
		}

		IndexType size() const noexcept
		{
			const std::shared_lock lock( m_mutex );
			return static_cast<IndexType>( m_handle_to_string.size() );
		}

	private:
		mutable std::shared_mutex m_mutex;
		std::unordered_map<std::string_view, handle> m_string_to_handle;
		std::vector<heap_string> m_handle_to_string;
	};
}
