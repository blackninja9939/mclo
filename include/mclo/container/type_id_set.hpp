#pragma once

#include "mclo/meta/type_id.hpp"

#include <unordered_set>

namespace mclo
{
	template <typename Container = std::unordered_set<meta::type_id_t>>
	class type_id_set
	{
	public:
		using key_type = typename Container::key_type;
		using value_type = typename Container::value_type;
		using reference = typename Container::reference;
		using const_reference = typename Container::const_reference;
		using size_type = typename Container::size_type;
		using difference_type = typename Container::difference_type;
		using iterator = typename Container::iterator;
		using const_iterator = typename Container::const_iterator;

		static_assert( std::is_same_v<key_type, meta::type_id_t>,
					   "type_id_set's key type must be mclo::meta::type_id_t" );

		template <typename T>
		bool insert()
		{
			return m_set.insert( meta::type_id<T> ).second;
		}

		template <typename T>
		bool erase() noexcept
		{
			return m_set.erase( meta::type_id<T> ) != 0;
		}

		template <typename T>
		[[nodiscard]] bool contains() const noexcept
		{
			return m_set.contains( meta::type_id<T> );
		}

		template <typename... Ts>
			requires( sizeof...( Ts ) > 1 )
		[[nodiscard]] bool contains() const noexcept
		{
			return ( contains<Ts>() && ... );
		}

		void clear() noexcept
		{
			m_set.clear();
		}

		void swap( type_id_set& other ) noexcept
		{
			m_set.swap( other.m_set );
		}

		friend void swap( type_id_set& lhs, type_id_set& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return m_set.empty();
		}
		[[nodiscard]] size_type size() const noexcept
		{
			return m_set.size();
		}
		[[nodiscard]] size_type max_size() const noexcept
		{
			return m_set.max_size();
		}

		[[nodiscard]] iterator begin() noexcept
		{
			return m_set.begin();
		}
		[[nodiscard]] const_iterator begin() const noexcept
		{
			return m_set.begin();
		}
		[[nodiscard]] const_iterator cbegin() const noexcept
		{
			return m_set.cbegin();
		}

		[[nodiscard]] iterator end() noexcept
		{
			return m_set.end();
		}
		[[nodiscard]] const_iterator end() const noexcept
		{
			return m_set.end();
		}
		[[nodiscard]] const_iterator cend() const noexcept
		{
			return m_set.cend();
		}

		[[nodiscard]] auto operator<=>( const type_id_set& other ) const noexcept = default;

	private:
		Container m_set;
	};
}
