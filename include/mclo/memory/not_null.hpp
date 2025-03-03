#pragma once

#include "mclo/debug/assert.hpp"
#include "mclo/hash/hash.hpp"

#include <compare>
#include <concepts>
#include <memory>
#include <type_traits>

namespace mclo
{
	template <typename Ptr>
	class not_null
	{
		template <typename U>
		friend class not_null;

	public:
		using element_type = Ptr;
		using underlying_value = typename std::pointer_traits<Ptr>::element_type;

		not_null() = delete;
		constexpr not_null( const not_null& other ) = default;
		constexpr not_null& operator=( const not_null& other ) = default;

		// Moving inherently nulls out smart pointers, so we want to make it very explicit as a member function
		constexpr not_null( not_null&& other ) = delete;
		constexpr not_null& operator=( not_null&& other ) = delete;

		template <std::convertible_to<Ptr> U>
		constexpr not_null( U&& ptr ) MCLO_NOEXCEPT_TESTS : m_ptr( std::forward<U>( ptr ) )
		{
			ASSUME( m_ptr != nullptr, "Constructing not_null with a null pointer", m_ptr );
		}

		template <std::convertible_to<Ptr> U>
		constexpr not_null& operator=( U&& ptr ) MCLO_NOEXCEPT_TESTS
		{
			ASSUME( ptr != nullptr, "Assigning not_null a null pointer", ptr );
			m_ptr = std::forward<U>( ptr );
			return *this;
		}

		template <std::convertible_to<Ptr> U>
		constexpr not_null( const not_null<U>& ptr ) MCLO_NOEXCEPT_TESTS : m_ptr( ptr.m_ptr )
		{
			ASSUME( m_ptr != nullptr, "Constructing from moved from not_null pointer", m_ptr );
		}
		template <std::convertible_to<Ptr> U>
		constexpr not_null& operator=( const not_null<U>& ptr ) MCLO_NOEXCEPT_TESTS
		{
			ASSUME( ptr.m_ptr != nullptr, "Assigning from moved from not_null pointer", ptr );
			m_ptr = ptr.m_ptr;
			return *this;
		}

		not_null( std::nullptr_t ) = delete;
		not_null& operator=( std::nullptr_t ) = delete;

		constexpr const element_type& get() const MCLO_NOEXCEPT_TESTS
		{
			return ASSUME_VAL( m_ptr, "Accessing moved from not_null pointer" );
		}

		constexpr const element_type& operator->() const noexcept
		{
			return get();
		}
		constexpr underlying_value& operator*() const noexcept
		{
			return *get();
		}

		constexpr Ptr unsafe_release() && MCLO_NOEXCEPT_TESTS
		{
			ASSUME( m_ptr != nullptr, "Releasing from moved from not_null pointer", m_ptr );
			return std::exchange( m_ptr, Ptr{} );
		}

		constexpr void swap( not_null& other )
		{
			using std::swap;
			swap( m_ptr, other.m_ptr );
		}

		friend constexpr void swap( not_null& lhs, not_null& rhs )
		{
			lhs.swap( rhs );
		}

		template <hasher Hasher>
		friend void hash_append( Hasher& hasher, const not_null& ptr ) noexcept
		{
			hash_append( hasher, ptr.get() );
		}

	private:
		Ptr m_ptr;
	};

	template <typename T, typename U>
		requires std::three_way_comparable_with<not_null<T>, not_null<U>>
	constexpr std::compare_three_way_result_t<not_null<T>, not_null<U>> operator<=>( const not_null<T>& lhs,
																					 const not_null<U>& rhs ) noexcept
	{
		return std::compare_three_way{}( lhs.get(), rhs.get() );
	}

	template <typename T, typename U>
	constexpr bool operator==( const not_null<T>& lhs, const not_null<U>& rhs ) noexcept
	{
		return lhs.get() == rhs.get();
	}

	template <typename T, typename Deleter = std::default_delete<T>>
	using not_null_unique_ptr = not_null<std::unique_ptr<T, Deleter>>;
}
