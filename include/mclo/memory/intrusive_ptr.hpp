#pragma once

#include "mclo/debug/assert.hpp"
#include "mclo/hash/hash.hpp"

#include <concepts>

namespace mclo
{
	/// @brief Tag type selecting the @ref intrusive_ptr constructor that adopts a pointer without adding a reference.
	struct maintain_refs_t
	{
		explicit maintain_refs_t() = default;
	};
	/// @brief Tag value selecting the @ref intrusive_ptr constructor that adopts a pointer without adding a reference.
	inline constexpr maintain_refs_t maintain_refs;

	/// @brief A smart pointer to an object that manages its own reference count, like @c boost::intrusive_ptr.
	/// @details The reference count lives in the pointee itself, located through the free functions
	/// @c intrusive_ptr_add_ref and @c intrusive_ptr_release_ref (found via ADL). @ref intrusive_ref_counter provides
	/// a ready-made thread-safe base. Unlike @c std::shared_ptr there is no separate control block, so a raw pointer
	/// can be converted to and from an @c intrusive_ptr freely.
	/// @tparam T The pointee type, which must supply the @c intrusive_ptr_add_ref / @c intrusive_ptr_release_ref hooks.
	template <typename T>
	class intrusive_ptr
	{
		template <typename U>
		friend class intrusive_ptr;

	public:
		/// @brief The pointee type.
		using element_type = T;

		/// @brief Constructs an empty (null) intrusive pointer.
		intrusive_ptr() noexcept = default;

		/// @brief Adopts @p ptr without incrementing its reference count, taking over an existing reference.
		/// @param ptr The pointer to adopt.
		intrusive_ptr( maintain_refs_t, T* const ptr ) noexcept
			: m_ptr( ptr )
		{
		}

		/// @brief Constructs an empty (null) intrusive pointer.
		intrusive_ptr( std::nullptr_t ) noexcept
		{
		}
		/// @brief Resets to null, releasing the current reference if any.
		intrusive_ptr& operator=( std::nullptr_t ) noexcept
		{
			reset();
			return *this;
		}

		/// @brief Points at @p ptr and increments its reference count.
		/// @param ptr The pointer to share ownership of.
		intrusive_ptr( T* const ptr ) noexcept
			: m_ptr( ptr )
		{
			add_ref();
		}
		/// @brief Points at @p ptr, incrementing its count and releasing the previous reference.
		/// @param ptr The pointer to share ownership of.
		intrusive_ptr& operator=( T* const ptr ) noexcept
		{
			reset( ptr );
			return *this;
		}

		/// @brief Copy constructor. Shares ownership and increments the reference count.
		intrusive_ptr( const intrusive_ptr& other ) noexcept
			: m_ptr( other.m_ptr )
		{
			add_ref();
		}
		/// @brief Copy assignment. Shares ownership of @p other, releasing the previous reference.
		intrusive_ptr& operator=( const intrusive_ptr& other ) noexcept
		{
			reset( other.m_ptr );
			return *this;
		}

		/// @brief Move constructor. Transfers ownership without touching the reference count, leaving @p other null.
		intrusive_ptr( intrusive_ptr&& other ) noexcept
			: m_ptr( other.detatch() )
		{
		}
		/// @brief Move assignment. Transfers ownership from @p other and releases the previous reference.
		intrusive_ptr& operator=( intrusive_ptr&& other ) noexcept
		{
			if ( this != &other )
			{
				reset_maintain_refs( other.detatch() );
			}
			return *this;
		}

		/// @brief Converting copy constructor from a pointer to a convertible type @p U.
		template <typename U>
			requires( std::convertible_to<U*, T*> )
		intrusive_ptr( const intrusive_ptr<U>& other ) noexcept
			: m_ptr( other.m_ptr )
		{
			add_ref();
		}
		/// @brief Converting copy assignment from a pointer to a convertible type @p U.
		template <typename U>
			requires( std::convertible_to<U*, T*> )
		intrusive_ptr& operator=( const intrusive_ptr<U>& other ) noexcept
		{
			reset( other.m_ptr );
			return *this;
		}

		/// @brief Converting move constructor from a pointer to a convertible type @p U.
		template <typename U>
			requires( std::convertible_to<U*, T*> )
		intrusive_ptr( intrusive_ptr<U>&& other ) noexcept
			: m_ptr( other.detatch() )
		{
		}
		/// @brief Converting move assignment from a pointer to a convertible type @p U.
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

		/// @brief Destructor. Releases the held reference if any.
		~intrusive_ptr()
		{
			if ( m_ptr )
			{
				intrusive_ptr_release_ref( m_ptr );
			}
		}

		/// @brief Returns the raw stored pointer without affecting the reference count.
		[[nodiscard]] T* get() const noexcept
		{
			return m_ptr;
		}

		/// @brief Accesses members of the pointee.
		[[nodiscard]] T* operator->() const noexcept
		{
			return m_ptr;
		}

		/// @brief Dereferences the pointee.
		/// @pre The pointer must be non-null.
		[[nodiscard]] T& operator*() const noexcept
		{
			MCLO_ASSUME( m_ptr, "Dereferencing null intrusive_ptr" );
			return *m_ptr;
		}

		/// @brief Returns true if the pointer is non-null.
		explicit operator bool() const noexcept
		{
			return m_ptr != nullptr;
		}

		/// @brief Releases the held reference and becomes null.
		void reset() noexcept
		{
			if ( m_ptr )
			{
				intrusive_ptr_release_ref( m_ptr );
				m_ptr = nullptr;
			}
		}

		/// @brief Releases the held reference and becomes null.
		void reset( std::nullptr_t ) noexcept
		{
			reset();
		}

		/// @brief Points at @p ptr, incrementing its count and releasing the previous reference.
		/// @param ptr The pointer to share ownership of.
		void reset( T* ptr ) noexcept
		{
			// If same pointer then we'd decrement our ref count, then increment it again, so
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

		/// @brief Adopts @p ptr without incrementing its count, releasing the previous reference.
		/// @details Use to take over an existing reference (for example one returned by @ref detatch).
		/// @param ptr The pointer whose reference to take over.
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

		/// @brief Releases ownership of the pointer without decrementing its count and becomes null.
		/// @return The previously held pointer; the caller takes over its reference.
		T* detatch() noexcept
		{
			return std::exchange( m_ptr, nullptr );
		}

		/// @brief Swaps the held pointers with @p other.
		void swap( intrusive_ptr& other ) noexcept
		{
			std::swap( m_ptr, other.m_ptr );
		}

		/// @brief Swaps two intrusive pointers.
		friend void swap( intrusive_ptr& lhs, intrusive_ptr& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

		/// @brief Orders against another intrusive pointer by the underlying pointer value.
		template <typename U>
		auto operator<=>( const intrusive_ptr<U>& rhs ) const noexcept
		{
			return std::compare_three_way{}( m_ptr, rhs.m_ptr );
		}
		/// @brief Compares against another intrusive pointer for pointer equality.
		template <typename U>
		bool operator==( const intrusive_ptr<U>& rhs ) const noexcept
		{
			return m_ptr == rhs.m_ptr;
		}

		/// @brief Orders against a raw pointer by pointer value.
		template <typename U>
		auto operator<=>( U* const rhs ) const noexcept
		{
			return std::compare_three_way{}( m_ptr, rhs );
		}
		/// @brief Compares against a raw pointer for equality.
		template <typename U>
		bool operator==( U* const rhs ) const noexcept
		{
			return m_ptr == rhs;
		}

		/// @brief Orders against null.
		auto operator<=>( std::nullptr_t ) const noexcept
		{
			return std::compare_three_way{}( m_ptr, static_cast<T*>( nullptr ) );
		}
		/// @brief Returns true if the pointer is null.
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
