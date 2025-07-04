#pragma once

#include "mclo/container/intrusive_forward_list_hook.hpp"

#include <concepts>
#include <iterator>
#include <type_traits>

namespace mclo
{
	template <typename T, typename Tag>
	class intrusive_forward_list;

	template <typename T, typename Tag>
	class intrusive_forward_list_iterator
	{
		using hook_type = intrusive_forward_list_hook<Tag>;
		static_assert( std::derived_from<T, hook_type>, "T must be derived from the intrusive list hook" );

		static_assert( std::is_object_v<T> );

		template <typename, typename>
		friend class intrusive_forward_list_iterator;

		friend class intrusive_forward_list<std::remove_const_t<T>, Tag>;

		template <typename U>
		static constexpr bool same_type = std::same_as<std::remove_const_t<T>, std::remove_const_t<U>>;

	public:
		using value_type = std::remove_const_t<T>;
		using difference_type = std::ptrdiff_t;
		using reference = T&;
		using pointer = T*;
		using iterator_category = std::forward_iterator_tag;
		using iterator_concept = std::forward_iterator_tag;

		intrusive_forward_list_iterator() = default;

		explicit intrusive_forward_list_iterator( const pointer data ) noexcept
			: m_data( data )
		{
		}

		template <typename U>
			requires( same_type<U> )
		intrusive_forward_list_iterator( const intrusive_forward_list_iterator<U, Tag>& other ) noexcept
			: m_data( const_cast<pointer>( other.m_data ) )
		{
		}

		[[nodiscard]] reference operator*() const noexcept
		{
			return *m_data;
		}

		[[nodiscard]] pointer operator->() const noexcept
		{
			return m_data;
		}

		intrusive_forward_list_iterator& operator++() noexcept
		{
			m_data = next( *m_data );
			return *this;
		}

		intrusive_forward_list_iterator operator++( int ) noexcept
		{
			intrusive_forward_list_iterator temp( *this );
			++( *this );
			return temp;
		}

		template <typename U>
			requires( same_type<U> )
		[[nodiscard]] bool operator==( const intrusive_forward_list_iterator<U, Tag>& other ) const noexcept
		{
			return m_data == other.m_data;
		}

	private:
		[[nodiscard]] static pointer next( const value_type& value ) noexcept
		{
			return static_cast<pointer>( static_cast<const hook_type&>( value ).m_next );
		}

		pointer m_data = nullptr;
	};
}
