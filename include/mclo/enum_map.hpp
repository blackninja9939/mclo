#pragma once

#include "mclo/array.hpp"
#include "mclo/detail/enum_container.hpp"

#include <ranges>
#include <type_traits>

namespace mclo
{
	template <typename TEnum, typename TValue, TEnum SizeEnum = enum_size<TEnum>>
	class enum_map
	{
		static_assert( detail::to_underlying( SizeEnum ) >= 0, "SizeEnum cannot have a negative value" );
		static constexpr std::size_t max_size = static_cast<std::size_t>( SizeEnum );
		using container_type = std::array<TValue, max_size>;

	public:
		static_assert( std::is_enum_v<TEnum>, "TEnum must be an enumeration type" );

		using key_type = TEnum;
		using value_type = TValue;
		using size_type = typename container_type::size_type;
		using difference_type = typename container_type::difference_type;
		using reference = typename container_type::reference;
		using const_reference = typename container_type::const_reference;
		using pointer = typename container_type::pointer;
		using const_pointer = typename container_type::const_pointer;
		using iterator = typename container_type::iterator;
		using const_iterator = typename container_type::const_iterator;
		using reverse_iterator = typename container_type::reverse_iterator;
		using const_reverse_iterator = typename container_type::const_reverse_iterator;

		template <bool IsConst>
		class enumerate_view;

		template <bool IsConst>
		class enumerate_iterator
		{
		public:
			friend class enumerate_view<IsConst>;

			using base_iterator = std::conditional_t<IsConst, enum_map::const_iterator, enum_map::iterator>;
			using base_reference = std::conditional_t<IsConst, enum_map::const_reference, enum_map::reference>;
			using iterator_category = std::random_access_iterator_tag;
			using iterator_concept = std::random_access_iterator_tag;
			using difference_type = enum_map::difference_type;
			using value_type = std::pair<key_type, value_type>;
			using reference = std::pair<key_type, base_reference>;

			constexpr enumerate_iterator() noexcept = default;

			constexpr enumerate_iterator( const enumerate_iterator<!IsConst> other ) noexcept
				requires IsConst
				: m_it( other.m_it )
				, m_index( other.m_index )
			{
			}

			constexpr reference operator*() const noexcept
			{
				return reference{ static_cast<key_type>( m_index ), *m_it };
			}

			constexpr enumerate_iterator& operator++() noexcept
			{
				++m_it;
				++m_index;
				return *this;
			}

			constexpr enumerate_iterator operator++( int ) noexcept
			{
				enumerate_iterator copy( *this );
				++*this;
				return copy;
			}

			constexpr enumerate_iterator& operator--() noexcept
			{
				--m_it;
				--m_index;
				return *this;
			}

			constexpr enumerate_iterator operator--( int ) noexcept
			{
				enumerate_iterator copy( *this );
				--*this;
				return copy;
			}

			constexpr enumerate_iterator& operator+=( const difference_type diff ) noexcept
			{
				m_it += diff;
				m_index += diff;
				return *this;
			}

			constexpr enumerate_iterator& operator-=( const difference_type diff ) noexcept
			{
				m_it -= diff;
				m_index -= diff;
				return *this;
			}

			[[nodiscard]] constexpr friend std::strong_ordering operator<=>( const enumerate_iterator& lhs,
																			 const enumerate_iterator& rhs ) noexcept
			{
				return lhs.m_index <=> rhs.m_index;
			}
			[[nodiscard]] constexpr friend bool operator==( const enumerate_iterator& lhs,
															const enumerate_iterator& rhs ) noexcept
			{
				return lhs.m_index == rhs.m_index;
			}

			[[nodiscard]] constexpr friend enumerate_iterator operator+( const enumerate_iterator& it,
																		 const difference_type diff ) noexcept
			{
				auto temp = it;
				temp += diff;
				return temp;
			}
			[[nodiscard]] constexpr friend enumerate_iterator operator+( const difference_type diff,
																		 const enumerate_iterator& it ) noexcept
			{
				return it + diff;
			}
			[[nodiscard]] constexpr friend enumerate_iterator operator-( const enumerate_iterator& it,
																		 const difference_type diff ) noexcept
			{
				auto temp = it;
				temp -= diff;
				return temp;
			}
			[[nodiscard]] constexpr friend difference_type operator+( const enumerate_iterator& lhs,
																	  const enumerate_iterator& rhs ) noexcept
			{
				return lhs.m_index - rhs.m_index;
			}

		private:
			constexpr enumerate_iterator( base_iterator it, const difference_type index ) noexcept
				: m_it( std::move( it ) )
				, m_index( index )
			{
			}

			base_iterator m_it;
			difference_type m_index;
		};

		template <bool IsConst>
		class enumerate_view : public std::ranges::view_interface<enumerate_view<IsConst>>
		{
			using parent = std::conditional_t<IsConst, const enum_map, enum_map>;
			using iterator = enumerate_iterator<IsConst>;

		public:
			constexpr explicit enumerate_view( parent& parent ) noexcept
				: m_parent( &parent )
			{
			}

			[[nodiscard]] constexpr iterator begin() const noexcept
			{
				return iterator( m_parent->begin(), 0 );
			}
			[[nodiscard]] constexpr iterator end() const noexcept
			{
				return iterator( m_parent->end(), max_size );
			}
			[[nodiscard]] constexpr size_type size() const noexcept
			{
				return max_size;
			}

		private:
			parent* const m_parent = nullptr;
		};

		constexpr enum_map() noexcept( std::is_nothrow_default_constructible_v<value_type> ) = default;

		constexpr explicit enum_map( const_reference fill_value ) noexcept(
			std::is_nothrow_copy_constructible_v<value_type> )
			: m_container( mclo::broadcast_array<max_size>( fill_value ) )
		{
		}

		template <typename... Ts>
			requires( max_size > 1 && sizeof...( Ts ) == max_size )
		constexpr enum_map( Ts&&... values ) noexcept( std::is_nothrow_move_constructible_v<value_type> )
			: m_container{ std::forward<Ts>( values )... }
		{
		}

		// Map like API

		[[nodiscard]] constexpr reference operator[]( const key_type key ) noexcept
		{
			return index_direct( static_cast<size_type>( key ) );
		}
		[[nodiscard]] constexpr const_reference operator[]( const key_type key ) const noexcept
		{
			return index_direct( static_cast<size_type>( key ) );
		}

		[[nodiscard]] constexpr reference index_direct( const size_type index ) noexcept
		{
			return m_container[ index ];
		}
		[[nodiscard]] constexpr const_reference index_direct( const size_type index ) const noexcept
		{
			return m_container[ index ];
		}

		[[nodiscard]] constexpr enumerate_view<false> enumerate() noexcept
		{
			return enumerate_view<false>{ *this };
		}
		[[nodiscard]] constexpr enumerate_view<true> enumerate() const noexcept
		{
			return enumerate_view<true>{ *this };
		}

		// std::array like API

		constexpr void fill( const_reference value ) noexcept( std::is_nothrow_copy_assignable_v<value_type> )
		{
			m_container.fill( value );
		}

		constexpr void swap( enum_map& other ) noexcept( std::is_nothrow_swappable_v<value_type> )
		{
			m_container.swap( other.m_container );
		}

		friend constexpr void swap( enum_map& lhs, enum_map& rhs ) noexcept( std::is_nothrow_swappable_v<value_type> )
		{
			lhs.swap( rhs );
		}

		[[nodiscard]] constexpr auto operator<=>( const enum_map& other ) const = default;

		[[nodiscard]] static constexpr size_type size() noexcept
		{
			return max_size;
		}

		[[nodiscard]] constexpr reference front() noexcept
		{
			return m_container.front();
		}
		[[nodiscard]] constexpr const_reference front() const noexcept
		{
			return m_container.front();
		}

		[[nodiscard]] constexpr reference back() noexcept
		{
			return m_container.back();
		}
		[[nodiscard]] constexpr const_reference back() const noexcept
		{
			return m_container.back();
		}

		[[nodiscard]] constexpr pointer data() noexcept
		{
			return m_container.data();
		}
		[[nodiscard]] constexpr const_pointer data() const noexcept
		{
			return m_container.data();
		}

		[[nodiscard]] constexpr iterator begin() noexcept
		{
			return m_container.begin();
		}
		[[nodiscard]] constexpr const_iterator begin() const noexcept
		{
			return m_container.begin();
		}
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept
		{
			return m_container.cbegin();
		}

		[[nodiscard]] constexpr iterator end() noexcept
		{
			return m_container.end();
		}
		[[nodiscard]] constexpr const_iterator end() const noexcept
		{
			return m_container.end();
		}
		[[nodiscard]] constexpr const_iterator cend() const noexcept
		{
			return m_container.cend();
		}

		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept
		{
			return m_container.rbegin();
		}
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept
		{
			return m_container.rbegin();
		}
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
		{
			return m_container.crbegin();
		}

		[[nodiscard]] constexpr reverse_iterator rend() noexcept
		{
			return m_container.rend();
		}
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept
		{
			return m_container.rend();
		}
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
		{
			return m_container.crend();
		}

	private:
		container_type m_container{};
	};
}
