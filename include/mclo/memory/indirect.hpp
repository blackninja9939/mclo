#pragma once

#include "mclo/debug/assert.hpp"
#include "mclo/preprocessor/platform.hpp"
#include "mclo/utility/synth_three_way.hpp"

#include <compare>
#include <concepts>
#include <memory>
#include <type_traits>

namespace mclo
{
#ifdef __cpp_lib_indirect
	using std::indirect;
#else
	template <typename T, typename Allocator = std::allocator<T>>
	class indirect
	{
		using alloc_traits = std::allocator_traits<Allocator>;
		static_assert( std::is_same_v<T, typename alloc_traits::value_type>, "Allocator::value_type must be T" );
		static_assert( std::is_object_v<T>, "T must be an object type" );
		static_assert( !std::is_same_v<T, std::in_place_t>, "T cannot be std::in_place_t" );
		static_assert( !std::is_same_v<T, std::in_place_type_t<T>>, "T cannot be std::in_place_type_t<T>" );
		static_assert( std::is_same_v<T, std::remove_cv_t<T>>, "T cannot be cv qualified" );

	public:
		using value_type = T;
		using allocator_type = Allocator;
		using pointer = typename alloc_traits::pointer;
		using const_pointer = typename alloc_traits::const_pointer;

		explicit constexpr indirect()
			requires( std::is_default_constructible_v<allocator_type> )
			: indirect( std::allocator_arg, allocator_type{} )
		{
			static_assert( std::is_default_constructible_v<T>, "T must be default constructible" );
		}

		explicit constexpr indirect( std::allocator_arg_t, const allocator_type& alloc )
			: m_alloc( alloc )
		{
			static_assert( std::is_default_constructible_v<T>, "T must be default constructible" );
			m_ptr = create( m_alloc );
		}

		constexpr indirect( const indirect& other )
			: indirect(
				  std::allocator_arg, alloc_traits::select_on_container_copy_construction( other.m_alloc ), other )
		{
			static_assert( std::is_copy_constructible_v<T>, "T must be copy constructible" );
		}

		constexpr indirect( std::allocator_arg_t, const Allocator& alloc, const indirect& other )
			: m_alloc( alloc )
		{
			static_assert( std::is_copy_constructible_v<T>, "T must be copy constructible" );
			if ( !other.valueless_after_move() )
			{
				m_ptr = create( m_alloc, *other.m_ptr );
			}
		}

		constexpr indirect( indirect&& other ) noexcept
			: indirect( std::allocator_arg, other.m_alloc, std::move( other ) )
		{
		}

		constexpr indirect( std::allocator_arg_t,
							const Allocator& alloc,
							indirect&& other ) noexcept( alloc_traits::is_always_equal::value )
			: m_alloc( alloc )
		{
			using std::swap;
			if constexpr ( alloc_traits::is_always_equal::value )
			{
				swap( m_ptr, other.m_ptr );
			}
			else
			{
				static_assert( sizeof( T ) != 0, "T must be complete" );
				if ( m_alloc == other.m_alloc )
				{
					swap( m_ptr, other.m_ptr );
				}
				else if ( !other.valueless_after_move() )
				{
					m_ptr = create( m_alloc, std::move( *other.m_ptr ) );
				}
			}
		}

		template <typename U = T>
		explicit constexpr indirect( U&& value )
			requires( !std::is_same_v<std::remove_cvref_t<U>, indirect> &&
					  !std::is_same_v<std::remove_cvref_t<U>, std::in_place_t> && std::is_constructible_v<T, U> &&
					  std::is_default_constructible_v<allocator_type> )
			: indirect( std::allocator_arg, allocator_type{}, std::forward<U>( value ) )
		{
		}

		template <typename U = T>
		explicit constexpr indirect( std::allocator_arg_t, const Allocator& alloc, U&& value )
			requires( !std::is_same_v<std::remove_cvref_t<U>, indirect> &&
					  !std::is_same_v<std::remove_cvref_t<U>, std::in_place_t> && std::is_constructible_v<T, U> )
			: m_alloc( alloc )
		{
			m_ptr = create( m_alloc, std::forward<U>( value ) );
		}

		template <typename... Us>
			requires( std::is_constructible_v<T, Us...> && std::is_default_constructible_v<allocator_type> )
		explicit constexpr indirect( std::in_place_t, Us&&... us )
			: indirect( std::allocator_arg, allocator_type{}, std::in_place, std::forward<Us>( us )... )
		{
		}

		template <typename... Us>
			requires( std::is_constructible_v<T, Us...> )
		explicit constexpr indirect( std::allocator_arg_t, const Allocator& alloc, std::in_place_t, Us&&... us )
			: m_alloc( alloc )
		{
			m_ptr = create( m_alloc, std::forward<Us>( us )... );
		}

		template <typename I, typename... Us>
			requires( std::is_constructible_v<T, std::initializer_list<I>&, Us...> &&
					  std::is_default_constructible_v<allocator_type> )
		explicit constexpr indirect( std::in_place_t, std::initializer_list<I> init_list, Us&&... us )
			: indirect( std::allocator_arg, allocator_type{}, std::in_place, init_list, std::forward<Us>( us )... )
		{
		}

		template <typename I, typename... Us>
			requires( std::is_constructible_v<T, std::initializer_list<I>&, Us...> )
		explicit constexpr indirect( std::allocator_arg_t,
									 const Allocator& alloc,
									 std::in_place_t,
									 std::initializer_list<I> init_list,
									 Us&&... us )
			: m_alloc( alloc )
		{
			m_ptr = create( m_alloc, init_list, std::forward<Us>( us )... );
		}

		constexpr ~indirect()
		{
			static_assert( std::is_destructible_v<T>, "T must be destructible" );
			static_assert( sizeof( T ) != 0, "T must be complete" );
			destroy();
		}

		constexpr indirect& operator=( const indirect& other )
		{
			static_assert( std::is_copy_assignable_v<T>, "T must be copy assignable" );
			static_assert( std::is_copy_constructible_v<T>, "T must be copy constructible" );

			if ( this == &other )
			{
				return *this;
			}

			constexpr bool update_alloc = alloc_traits::propagate_on_container_copy_assignment::value;
			if ( other.valueless_after_move() )
			{
				destroy();
			}
			else
			{
				if ( std::assignable_from<T&, T> && !valueless_after_move() && m_alloc == other.m_alloc )
				{
					*m_ptr = *other.m_ptr;
				}
				else
				{
					pointer tmp = create_select_alloc<update_alloc>( other.m_alloc, m_alloc, *other.m_ptr );
					destroy();
					m_ptr = tmp;
				}
			}

			if ( update_alloc )
			{
				m_alloc = other.m_alloc;
			}

			return *this;
		}

		constexpr indirect& operator=( indirect&& other ) noexcept(
			alloc_traits::propagate_on_container_move_assignment::value || alloc_traits::is_always_equal::value )
		{
			static_assert( std::is_copy_constructible_v<T>, "T must be copy constructible" );

			if ( this == &other )
			{
				return *this;
			}

			constexpr bool update_alloc = alloc_traits::propagate_on_container_move_assignment::value;
			if ( other.valueless_after_move() )
			{
				destroy();
			}
			else
			{
				if ( m_alloc == other.m_alloc )
				{
					using std::swap;
					swap( m_ptr, other.m_ptr );
					other.destroy();
				}
				else
				{
					pointer tmp =
						create_select_alloc<update_alloc>( other.m_alloc, m_alloc, std::move( *other.m_ptr ) );
					destroy();
					m_ptr = tmp;
				}
			}

			if ( update_alloc )
			{
				m_alloc = other.m_alloc;
			}

			return *this;
		}

		template <typename U = T>
			requires( !std::is_same_v<std::remove_cvref_t<U>, indirect> && std::is_constructible_v<T, U> &&
					  std::is_assignable_v<T&, U> )
		constexpr indirect& operator=( U&& other )
		{
			if ( valueless_after_move() )
			{
				m_ptr = create( m_alloc, std::forward<U>( other ) );
			}
			else
			{
				*m_ptr = std::forward<U>( other );
			}
			return *this;
		}

		[[nodiscard]] constexpr const T& operator*() const& noexcept
		{
			return *m_ptr;
		}

		[[nodiscard]] constexpr T& operator*() & noexcept
		{
			return *m_ptr;
		}

		[[nodiscard]] constexpr const T&& operator*() const&& noexcept
		{
			return std::move( **this );
		}

		[[nodiscard]] constexpr T&& operator*() && noexcept
		{
			return std::move( **this );
		}

		[[nodiscard]] constexpr const_pointer operator->() const noexcept
		{
			return m_ptr;
		}

		[[nodiscard]] constexpr pointer operator->() noexcept
		{
			return m_ptr;
		}

		constexpr bool valueless_after_move() const noexcept
		{
			return m_ptr == nullptr;
		}

		[[nodiscard]] constexpr allocator_type get_allocator() const noexcept
		{
			return m_alloc;
		}

		constexpr void swap( indirect& other ) noexcept( alloc_traits::propagate_on_container_swap::value ||
														 alloc_traits::is_always_equal::value )
		{
			using std::swap;

			if constexpr ( alloc_traits::propagate_on_container_swap::value )
			{
				swap( m_alloc, other.m_alloc );
				swap( m_ptr, other.m_ptr );
			}
			else
			{
				if ( m_alloc == other.m_alloc )
				{
					swap( m_ptr, other.m_ptr );
				}
				else
				{
					UNREACHABLE( "Cannot swap indirects with different allocators" );
				}
			}
		}

		friend constexpr void swap( indirect& lhs, indirect& rhs ) noexcept( noexcept( lhs.swap( rhs ) ) )
		{
			lhs.swap( rhs );
		}

		template <class U, class AA>
		[[nodiscard]] friend constexpr bool operator==( const indirect& lhs,
														const indirect<U, AA>& rhs ) noexcept( noexcept( *lhs ==
																										 *rhs ) )
		{
			if ( lhs.valueless_after_move() )
			{
				return rhs.valueless_after_move();
			}
			else if ( rhs.valueless_after_move() )
			{
				return false;
			}
			else
			{
				return *lhs == *rhs;
			}
		}

		template <class U, class AA>
		[[nodiscard]] friend constexpr auto operator<=>( const indirect& lhs, const indirect<U, AA>& rhs ) noexcept(
			noexcept( *lhs <=> *rhs ) ) -> mclo::synth_three_way_result<T, U>
		{
			if ( lhs.valueless_after_move() || rhs.valueless_after_move() )
			{
				return !lhs.valueless_after_move() <=> !rhs.valueless_after_move();
			}
			else
			{
				return mclo::synth_three_way{}( *lhs, *rhs );
			}
		}

		template <class U>
		[[nodiscard]] friend constexpr bool operator==( const indirect& lhs,
														const U& rhs ) noexcept( noexcept( *lhs == rhs ) )
		{
			if ( lhs.valueless_after_move() )
			{
				return false;
			}
			else
			{
				return *lhs == rhs;
			}
		}

		template <class U>
		[[nodiscard]] friend constexpr auto operator<=>( const indirect& lhs,
														 const U& rhs ) noexcept( noexcept( *lhs <=> rhs ) )
			-> mclo::synth_three_way_result<T, U>
		{
			if ( lhs.valueless_after_move() )
			{
				return std::strong_ordering::less;
			}
			else
			{
				return mclo::synth_three_way{}( *lhs, rhs );
			}
		}

	private:
		constexpr void destroy()
		{
			if ( m_ptr )
			{
				alloc_traits::destroy( m_alloc, m_ptr );
				alloc_traits::deallocate( m_alloc, m_ptr, 1 );
				m_ptr = nullptr;
			}
		}

		template <typename... Ts>
		[[nodiscard]] constexpr static pointer create( allocator_type& allocator, Ts&&... ts )
		{
			pointer ptr = alloc_traits::allocate( allocator, 1 );
			try
			{
				alloc_traits::construct( allocator, ptr, std::forward<Ts>( ts )... );
				return ptr;
			}
			catch ( ... )
			{
				alloc_traits::deallocate( allocator, ptr, 1 );
				throw;
			}
		}

		template <bool update_alloc, typename... Ts>
		[[nodiscard]] constexpr static pointer create_select_alloc( const allocator_type& updated_alloc,
																	allocator_type& allocator,
																	Ts&&... ts )
		{
			if constexpr ( update_alloc )
			{
				allocator_type alloc( updated_alloc );
				return create( alloc, std::forward<Ts>( ts )... );
			}
			else
			{
				return create( allocator, std::forward<Ts>( ts )... );
			}
		}

		pointer m_ptr = nullptr;
		MCLO_NO_UNIQUE_ADDRESS allocator_type m_alloc;
	};

	template <typename T>
	indirect( T ) -> indirect<T>;

	template <typename T, typename Allocator>
	indirect( std::allocator_arg_t, Allocator, T )
		-> indirect<T, typename std::allocator_traits<Allocator>::template rebind_alloc<T>>;
#endif
}

#ifndef __cpp_lib_indirect
template <typename T, typename Allocator>
struct std::hash<mclo::indirect<T, Allocator>>
{
	std::size_t operator()( const mclo::indirect<T, Allocator>& value ) const noexcept
	{
		if ( value.valueless_after_move() )
		{
			return 42;
		}
		return std::hash<T>{}( *value );
	}
};
#endif
