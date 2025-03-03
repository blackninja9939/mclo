#pragma once

#include "mclo/debug/assert.hpp"
#include "mclo/memory/tagged_ptr.hpp"
#include "mclo/meta/count.hpp"
#include "mclo/meta/index_of.hpp"
#include "mclo/meta/nth_type.hpp"
#include "mclo/meta/type_list.hpp"

#include <array>
#include <functional>

namespace mclo
{
	template <typename... Ts>
	class pointer_variant
	{
	public:
		static constexpr std::size_t size = sizeof...( Ts );

	private:
		using ptr_type = tagged_ptr<const void, std::uint8_t, std::max( { alignof( Ts )... } )>;

		static_assert( size > 0, "Must have at least one alternative" );
		static_assert( ptr_type::can_store_tag( size - 1 ), "Too many alternatives in pointer_variant" );

		using ptr_alternatives = meta::type_list<Ts...>;

		template <typename T>
		static constexpr std::size_t tag_v = meta::index_of_v<T, ptr_alternatives>;

	public:
		template <std::size_t Index>
		using alternative_t = meta::nth<Index, ptr_alternatives>;

		constexpr pointer_variant() noexcept = default;

		template <typename T>
		explicit pointer_variant( T* const ptr ) noexcept
			: m_ptr( ptr, tag_v<T> )
		{
		}

		template <typename T, std::size_t Index>
		explicit pointer_variant( std::in_place_index_t<Index>, T* const ptr ) noexcept
			: m_ptr( ptr, Index )
		{
			static_assert( Index < size, "Index out of range of alternatives" );
			static_assert( std::is_same_v<std::decay_t<T>, alternative_t<Index>>,
						   "Type at index is not type of the pointer" );
		}

		template <typename T>
		pointer_variant& operator=( T* const ptr ) noexcept
		{
			emplace( ptr );
			return *this;
		}

		[[nodiscard]] constexpr std::size_t index() const noexcept
		{
			return m_ptr.tag();
		}

		template <typename T>
		[[nodiscard]] bool holds_alternative() const noexcept
		{
			static_assert(
				meta::count_v<T, ptr_alternatives> == 1,
				"holds_alternative is only valid to use if T appears in the alternative types exactly once" );
			return index() == tag_v<T>;
		}

		template <typename T>
		void emplace( T* const ptr ) noexcept
		{
			m_ptr.reset( ptr, tag_v<T> );
		}

		[[nodiscard]] void* get_raw() noexcept
		{
			return const_cast<void*>( m_ptr.get() );
		}
		[[nodiscard]] const void* get_raw() const noexcept
		{
			return m_ptr.get();
		}

		template <typename T>
		[[nodiscard]] T* get() MCLO_NOEXCEPT_TESTS
		{
			static_assert( meta::count_v<T, ptr_alternatives> == 1,
						   "Type based get is only valid to use if T appears in the alternative types exactly once" );
			DEBUG_ASSERT( index() == tag_v<T>, "Variant does not currently hold altnerative" );
			return cast_to<T>( get_raw() );
		}
		template <typename T>
		[[nodiscard]] const T* get() const MCLO_NOEXCEPT_TESTS
		{
			static_assert( meta::count_v<T, ptr_alternatives> == 1,
						   "Type based get is only valid to use if T appears in the alternative types exactly once" );
			DEBUG_ASSERT( index() == tag_v<T>, "Variant does not currently hold altnerative" );
			return cast_to<const T>( get_raw() );
		}

		template <std::size_t Index>
		[[nodiscard]] alternative_t<Index>* get() MCLO_NOEXCEPT_TESTS
		{
			static_assert( Index < size, "Index out of range of alternatives" );
			DEBUG_ASSERT( index() == Index, "Variant does not currently hold altnerative" );
			return cast_to<alternative_t<Index>>( get_raw() );
		}
		template <std::size_t Index>
		[[nodiscard]] const alternative_t<Index>* get() const MCLO_NOEXCEPT_TESTS
		{
			static_assert( Index < size, "Index out of range of alternatives" );
			DEBUG_ASSERT( index() == Index, "Variant does not currently hold altnerative" );
			return cast_to<const alternative_t<Index>>( get_raw() );
		}

		template <typename Func>
		decltype( auto ) visit( Func&& func )
		{
			static constexpr std::array func_table{ &dispatch<Ts, Func>... };
			return std::invoke( func_table[ index() ], std::forward<Func>( func ), get_raw() );
		}
		template <typename Func>
		decltype( auto ) visit( Func&& func ) const
		{
			static constexpr std::array func_table{ &dispatch<const Ts, Func>... };
			return std::invoke( func_table[ index() ], std::forward<Func>( func ), get_raw() );
		}

		void swap( pointer_variant& other ) noexcept
		{
			m_ptr.swap( other.m_ptr );
		}

		friend void swap( pointer_variant& lhs, pointer_variant& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

	private:
		template <typename T>
		[[nodiscard]] static constexpr T* cast_to( const void* ptr ) noexcept
		{
			return const_cast<T*>( static_cast<std::add_const_t<T>*>( ptr ) );
		}

		template <typename T, typename Func>
		static constexpr std::invoke_result_t<Func, T*> dispatch( Func&& func, const void* const ptr )
		{
			return std::invoke( std::forward<Func>( func ), cast_to<T>( ptr ) );
		}

		ptr_type m_ptr;
	};
}
