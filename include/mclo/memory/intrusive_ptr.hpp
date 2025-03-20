#pragma once

#include "mclo/debug/assert.hpp"
#include "mclo/hash/hash.hpp"

#include <concepts>

namespace mclo
{
	struct maintain_refs_t
	{
		explicit maintain_refs_t() = default;	
	};
	inline constexpr maintain_refs_t maintain_refs;

	template <typename T>
	class intrusive_ptr
	{
		template <typename U>
		friend class intrusive_ptr;

	public:
		using element_type = T;

		intrusive_ptr() noexcept = default;

		intrusive_ptr( maintain_refs_t, T* const ptr ) noexcept
			: m_ptr( ptr )
		{
		}

		intrusive_ptr( std::nullptr_t ) noexcept
		{
		}
		intrusive_ptr& operator=( std::nullptr_t ) noexcept
		{
			reset();
			return *this;
		}

		intrusive_ptr( T* const ptr ) noexcept
			: m_ptr( ptr )
		{
			add_ref();
		}
		intrusive_ptr& operator=( T* const ptr ) noexcept
		{
			reset( ptr );
			return *this;
		}

		intrusive_ptr( const intrusive_ptr& other ) noexcept
			: m_ptr( other.m_ptr )
		{
			add_ref();
		}
		intrusive_ptr& operator=( const intrusive_ptr& other ) noexcept
		{
			reset( other.m_ptr );
			return *this;
		}

		intrusive_ptr( intrusive_ptr&& other ) noexcept
			: m_ptr( other.detatch() )
		{
		}
		intrusive_ptr& operator=( intrusive_ptr&& other ) noexcept
		{
			if ( this != &other )
			{
				reset_maintain_refs( other.detatch() );
			}
			return *this;
		}

		template <typename U>
			requires( std::convertible_to<U*, T*> )
		intrusive_ptr( const intrusive_ptr<U>& other ) noexcept
			: m_ptr( other.m_ptr )
		{
			add_ref();
		}
		template <typename U>
			requires( std::convertible_to<U*, T*> )
		intrusive_ptr& operator=( const intrusive_ptr<U>& other ) noexcept
		{
			reset( other.m_ptr );
			return *this;
		}

		template <typename U>
			requires( std::convertible_to<U*, T*> )
		intrusive_ptr( intrusive_ptr<U>&& other ) noexcept
			: m_ptr( other.detatch() )
		{
		}
		template <typename U>
			requires( std::convertible_to<U*, T*> )
		intrusive_ptr& operator=( intrusive_ptr<U>&& other ) noexcept
		{
			if ( this != &other )
			{
				reset_maintain_refs( other.detatch() );
			}
			return *this;
		}

		~intrusive_ptr()
		{
			if ( m_ptr )
			{
				intrusive_ptr_release_ref( m_ptr );
			}
		}

		[[nodiscard]] T* get() const noexcept
		{
			return m_ptr;
		}

		[[nodiscard]] T* operator->() const noexcept
		{
			return m_ptr;
		}

		[[nodiscard]] T& operator*() const noexcept
		{
			ASSUME( m_ptr, "Dereferencing null intrusive_ptr" );
			return *m_ptr;
		}

		explicit operator bool() const noexcept
		{
			return m_ptr != nullptr;
		}

		void reset() noexcept
		{
			if ( m_ptr )
			{
				intrusive_ptr_release_ref( m_ptr );
				m_ptr = nullptr;
			}
		}

		void reset( std::nullptr_t ) noexcept
		{
			reset();
		}

		void reset( T* ptr ) noexcept
		{
			// If same pointer then we'd decrememnt our ref count, then incrememnt it again, so
			// we can early out
			if ( m_ptr == ptr )
			{
				return;
			}

			if ( m_ptr )
			{
				intrusive_ptr_release_ref( m_ptr );
			}
			m_ptr = ptr;
			add_ref();
		}

		void reset_maintain_refs( T* ptr ) noexcept
		{
			// No early out as if ptr == m_ptr we still want to remove our current ref count
			// before taking over ownership of its
			if ( m_ptr )
			{
				intrusive_ptr_release_ref( m_ptr );
			}
			m_ptr = ptr;
		}

		T* detatch() noexcept
		{
			return std::exchange( m_ptr, nullptr );
		}

		void swap( intrusive_ptr& other ) noexcept
		{
			std::swap( m_ptr, other.m_ptr );
		}

		friend void swap( intrusive_ptr& lhs, intrusive_ptr& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

		template <typename U>
		auto operator<=>( const intrusive_ptr<U>& rhs ) const noexcept
		{
			return std::compare_three_way{}( m_ptr, rhs.m_ptr );
		}
		template <typename U>
		bool operator==( const intrusive_ptr<U>& rhs ) const noexcept
		{
			return m_ptr == rhs.m_ptr;
		}

		template <typename U>
		auto operator<=>( U* const rhs ) const noexcept
		{
			return std::compare_three_way{}( m_ptr, rhs );
		}
		template <typename U>
		bool operator==( U* const rhs ) const noexcept
		{
			return m_ptr == rhs;
		}

		auto operator<=>( std::nullptr_t ) const noexcept
		{
			return std::compare_three_way{}( m_ptr, static_cast<T*>( nullptr ) );
		}
		bool operator==( std::nullptr_t ) const noexcept
		{
			return !m_ptr;
		}

	private:
		void add_ref() noexcept
		{
			if ( m_ptr )
			{
				intrusive_ptr_add_ref( m_ptr );
			}
		}

		T* m_ptr = nullptr;
	};
}
