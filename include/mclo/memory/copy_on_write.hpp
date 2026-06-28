#pragma once

#include <memory>

#include "mclo/concepts/invocable_r.hpp"
#include "mclo/concepts/specialization_of.hpp"
#include "mclo/debug/assert.hpp"
#include "mclo/platform/attributes.hpp"
#include "mclo/utility/synth_three_way.hpp"

#include <atomic>
#include <compare>
#include <concepts>
#include <memory_resource>
#include <type_traits>

namespace mclo
{
	/// @brief A reference-counted copy-on-write value wrapper.
	/// @details Multiple @c copy_on_write instances can share ownership of the same underlying value.
	/// The contained value is only accessible through a const reference via @c operator* and @c operator->.
	/// To mutate the value, use @c modify() which automatically creates a private copy when the storage is shared,
	/// ensuring other holders are unaffected.
	///
	/// A @c copy_on_write may become valueless after being moved from. A valueless instance compares less than any
	/// non-valueless instance and equal to other valueless instances. The valueless state exists solely to enable
	/// efficient moves, using @c valueless_after_move() as a nullability check is an anti-pattern. If optional
	/// semantics are needed, wrap the @c copy_on_write in a @c std::optional instead.
	///
	/// @tparam T The type of the contained value. Must be a non-array, non-cv-qualified object type.
	/// @tparam Allocator The allocator type used for internal storage. Defaults to @c std::allocator<T>.
	/// @see mclo::indirect
	/// @see mclo::polymorphic
	template <typename T, typename Allocator = std::allocator<T>>
	class copy_on_write
	{
		using alloc_traits = std::allocator_traits<Allocator>;
		static_assert( std::is_same_v<T, typename alloc_traits::value_type>, "Allocator::value_type must be T" );
		static_assert( std::is_object_v<T>, "T must be an object type" );
		static_assert( !std::is_array_v<T>, "T must not be an array type" );
		static_assert( !std::is_same_v<T, std::in_place_t>, "T cannot be std::in_place_t" );
		static_assert( !mclo::specialization_of<T, std::in_place_type_t>,
					   "T cannot be a specialization of std::in_place_type_t" );
		static_assert( std::is_same_v<T, std::remove_cv_t<T>>, "T cannot be cv qualified" );

		template <typename U>
		static constexpr bool is_convertible_from =
			!std::is_same_v<std::remove_cvref_t<U>, copy_on_write> &&
			!std::is_same_v<std::remove_cvref_t<U>, std::in_place_t> && std::is_constructible_v<T, U>;

	public:
		/// @brief The type of the contained value.
		using value_type = T;

		/// @brief The allocator type.
		using allocator_type = Allocator;

		/// @brief A const pointer to the contained value, as provided by the allocator.
		using const_pointer = typename alloc_traits::const_pointer;

		/// @brief Constructs a @c copy_on_write with a default-constructed value.
		explicit copy_on_write()
			requires( std::is_default_constructible_v<allocator_type> )
			: copy_on_write( std::allocator_arg, allocator_type{} )
		{
			static_assert( std::is_default_constructible_v<T>, "T must be default constructible" );
		}

		/// @brief Constructs a @c copy_on_write with a default-constructed value using the given allocator.
		/// @param alloc The allocator to use.
		explicit copy_on_write( std::allocator_arg_t, const allocator_type& alloc )
			: m_alloc( alloc )
		{
			static_assert( std::is_default_constructible_v<T>, "T must be default constructible" );
			m_ptr = create( m_alloc );
		}

		/// @brief Copy constructor. Shares storage with @p other if allocators are compatible.
		copy_on_write( const copy_on_write& other )
			: copy_on_write(
				  std::allocator_arg, alloc_traits::select_on_container_copy_construction( other.m_alloc ), other )
		{
			static_assert( std::is_copy_constructible_v<T>, "T must be copy constructible" );
		}

		/// @brief Allocator-extended copy constructor.
		/// @details Shares storage with @p other if @p alloc compares equal to the other's allocator.
		/// Otherwise, creates a deep copy of the contained value using @p alloc.
		/// @param alloc The allocator to use.
		/// @param other The instance to copy from.
		copy_on_write( std::allocator_arg_t, const Allocator& alloc, const copy_on_write& other )
			: m_alloc( alloc )
		{
			static_assert( std::is_copy_constructible_v<T>, "T must be copy constructible" );
			if ( !other.valueless_after_move() )
			{
				if ( m_alloc == other.m_alloc )
				{
					m_ptr = other.m_ptr;
					m_ptr->m_counter.fetch_add( 1, std::memory_order_relaxed );
				}
				else
				{
					m_ptr = create( m_alloc, other.m_ptr->m_value );
				}
			}
		}

		/// @brief Move constructor. Leaves @p other in a valueless state.
		/// @post @p other is valueless.
		copy_on_write( copy_on_write&& other ) noexcept
			: m_alloc( std::move( other.m_alloc ) )
			, m_ptr( std::exchange( other.m_ptr, nullptr ) )
		{
		}

		/// @brief Allocator-extended move constructor.
		/// @details Steals storage from @p other if allocators are equal. Otherwise, creates new storage
		/// using @p alloc and moves the contained value. Leaves @p other in a valueless state.
		/// @param alloc The allocator to use.
		/// @param other The instance to move from.
		/// @post @p other is valueless.
		copy_on_write( std::allocator_arg_t,
					   const Allocator& alloc,
					   copy_on_write&& other ) noexcept( alloc_traits::is_always_equal::value )
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
					m_ptr = create( m_alloc, std::move( *other ) );
					other.destroy();
				}
			}
		}

		/// @brief Constructs a @c copy_on_write from a value convertible to @p T.
		/// @param value The value to store.
		template <typename U = T>
		explicit copy_on_write( U&& value )
			requires( is_convertible_from<U> && std::is_default_constructible_v<allocator_type> )
			: copy_on_write( std::allocator_arg, allocator_type{}, std::forward<U>( value ) )
		{
		}

		/// @brief Allocator-extended constructor from a value convertible to @p T.
		/// @param alloc The allocator to use.
		/// @param value The value to store.
		template <typename U = T>
		explicit copy_on_write( std::allocator_arg_t, const Allocator& alloc, U&& value )
			requires( is_convertible_from<U> )
			: m_alloc( alloc )
		{
			m_ptr = create( m_alloc, std::forward<U>( value ) );
		}

		/// @brief Constructs the contained value in place with the given arguments.
		/// @param us Arguments to forward to the constructor of @p T.
		template <typename... Us>
			requires( std::is_constructible_v<T, Us...> && std::is_default_constructible_v<allocator_type> )
		explicit copy_on_write( std::in_place_t, Us&&... us )
			: copy_on_write( std::allocator_arg, allocator_type{}, std::in_place, std::forward<Us>( us )... )
		{
		}

		/// @brief Allocator-extended in-place constructor.
		/// @param alloc The allocator to use.
		/// @param us Arguments to forward to the constructor of @p T.
		template <typename... Us>
			requires( std::is_constructible_v<T, Us...> )
		explicit copy_on_write( std::allocator_arg_t, const Allocator& alloc, std::in_place_t, Us&&... us )
			: m_alloc( alloc )
		{
			m_ptr = create( m_alloc, std::forward<Us>( us )... );
		}

		/// @brief Constructs the contained value in place from an initializer list and optional arguments.
		/// @param init_list The initializer list.
		/// @param us Additional arguments to forward to the constructor of @p T.
		template <typename I, typename... Us>
			requires( std::is_constructible_v<T, std::initializer_list<I>&, Us...> &&
					  std::is_default_constructible_v<allocator_type> )
		explicit copy_on_write( std::in_place_t, std::initializer_list<I> init_list, Us&&... us )
			: copy_on_write( std::allocator_arg, allocator_type{}, std::in_place, init_list, std::forward<Us>( us )... )
		{
		}

		/// @brief Allocator-extended in-place constructor from an initializer list.
		/// @param alloc The allocator to use.
		/// @param init_list The initializer list.
		/// @param us Additional arguments to forward to the constructor of @p T.
		template <typename I, typename... Us>
			requires( std::is_constructible_v<T, std::initializer_list<I>&, Us...> )
		explicit copy_on_write( std::allocator_arg_t,
								const Allocator& alloc,
								std::in_place_t,
								std::initializer_list<I> init_list,
								Us&&... us )
			: m_alloc( alloc )
		{
			m_ptr = create( m_alloc, init_list, std::forward<Us>( us )... );
		}

		/// @brief Destructor. Decrements the reference count and destroys the value if this was the last owner.
		~copy_on_write()
		{
			static_assert( std::is_destructible_v<T>, "T must be destructible" );
			static_assert( sizeof( T ) != 0, "T must be complete" );
			destroy();
		}

		/// @brief Copy assignment operator. Shares storage with @p other if allocators are compatible.
		/// @details If @p other is valueless, this instance becomes valueless. If allocators are equal
		/// (or propagated), shares the storage by incrementing the reference count. Otherwise, creates
		/// a deep copy of the contained value.
		copy_on_write& operator=( const copy_on_write& other )
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
			else if ( update_alloc || m_alloc == other.m_alloc )
			{
				other.m_ptr->m_counter.fetch_add( 1, std::memory_order_relaxed );
				destroy();
				m_ptr = other.m_ptr;
			}
			else
			{
				storage* tmp = create( m_alloc, *other );
				destroy();
				m_ptr = tmp;
			}

			if ( update_alloc )
			{
				m_alloc = other.m_alloc;
			}

			return *this;
		}

		/// @brief Move assignment operator. Steals storage from @p other if allocators are compatible.
		/// @details If @p other is valueless, this instance becomes valueless. If allocators are equal
		/// (or propagated), steals the pointer. Otherwise, creates new storage by moving the value
		/// and leaves @p other valueless.
		/// @post @p other is valueless.
		copy_on_write& operator=( copy_on_write&& other ) noexcept(
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
			else if ( update_alloc || m_alloc == other.m_alloc )
			{
				destroy();
				m_ptr = std::exchange( other.m_ptr, nullptr );
			}
			else
			{
				storage* tmp = create( m_alloc, std::move( *other ) );
				destroy();
				other.destroy();
				m_ptr = tmp;
			}

			if ( update_alloc )
			{
				m_alloc = other.m_alloc;
			}

			return *this;
		}

		/// @brief Value assignment operator.
		/// @details If unique, assigns the value in place. If shared or valueless, creates new storage.
		/// @param other The value to assign.
		template <typename U = T>
			requires( !std::is_same_v<std::remove_cvref_t<U>, copy_on_write> && std::is_constructible_v<T, U> &&
					  std::is_assignable_v<T&, U> )
		copy_on_write& operator=( U&& other )
		{
			if ( valueless_after_move() || m_ptr->m_counter.load( std::memory_order_acquire ) != 1 )
			{
				storage* tmp = create( m_alloc, std::forward<U>( other ) );
				destroy();
				m_ptr = tmp;
			}
			else
			{
				m_ptr->m_value = std::forward<U>( other );
			}
			return *this;
		}

		/// @brief Returns a const reference to the contained value.
		/// @pre The instance must not be valueless.
		[[nodiscard]] const T& operator*() const& noexcept
		{
			DEBUG_ASSERT( !valueless_after_move(), "Dereferencing copy_on_write that is valueless_after_move" );
			return m_ptr->m_value;
		}

		/// @brief Returns a const pointer to the contained value for member access.
		/// @pre The instance must not be valueless.
		[[nodiscard]] const_pointer operator->() const noexcept
		{
			DEBUG_ASSERT( !valueless_after_move(), "Getting pointer for copy_on_write that is valueless_after_move" );
			return std::addressof( m_ptr->m_value );
		}

		/// @brief Returns @c true if this instance has been moved from and no longer holds a value.
		[[nodiscard]] bool valueless_after_move() const noexcept
		{
			return m_ptr == nullptr;
		}

		/// @brief Returns a copy of the allocator.
		[[nodiscard]] allocator_type get_allocator() const noexcept
		{
			return m_alloc;
		}

		/// @brief Returns the number of @c copy_on_write instances sharing the same storage.
		/// @pre The instance must not be valueless.
		/// @note The returned value is a snapshot and may be stale by the time it is read in concurrent
		/// code. Do not use it for synchronization decisions.
		[[nodiscard]] long use_count() const noexcept
		{
			DEBUG_ASSERT( !valueless_after_move(), "Getting use_count for copy_on_write that is valueless_after_move" );
			return m_ptr->m_counter.load( std::memory_order_relaxed );
		}

		/// @brief Returns @c true if this instance and @p other share the same underlying storage.
		/// @pre Neither instance may be valueless.
		[[nodiscard]] bool identical_to( const copy_on_write& other ) const noexcept
		{
			DEBUG_ASSERT( !valueless_after_move() && !other.valueless_after_move(),
						  "Checking identical_to for copy_on_write that is valueless_after_move" );
			return m_ptr == other.m_ptr;
		}

		/// @brief Mutates the contained value, copying first if the storage is shared.
		/// @details If the reference count is greater than one, creates a private copy before
		/// invoking @p func. If unique, invokes @p func directly on the existing value.
		/// @param func A callable invoked as @c func(T&) to mutate the value.
		/// @pre The instance must not be valueless.
		/// @warning Calling @c modify() concurrently on the same instance is a data race and requires
		/// external synchronization. Pass the @c copy_on_write by value to each thread rather than
		/// sharing a reference, the copy-on-write invariant handles the rest.
		/// @warning Storing the reference passed to @p func beyond the callback's return is undefined
		/// behaviour. The reference may be invalidated by subsequent operations on this instance.
		template <typename Func>
			requires mclo::invocable_r<void, Func, T&>
		void modify( Func&& func )
		{
			DEBUG_ASSERT( !valueless_after_move(), "Modifying copy_on_write that is valueless_after_move" );

			if ( m_ptr->m_counter.load( std::memory_order_acquire ) > 1 )
			{
				storage* tmp = create( m_alloc, std::as_const( m_ptr->m_value ) );
				destroy();
				m_ptr = tmp;
			}

			std::forward<Func>( func )( m_ptr->m_value );
		}

		/// @brief Mutates the contained value with separate paths for shared and unique storage.
		/// @details If shared, invokes @p transform to construct a new value from the existing one.
		/// If unique, invokes @p func to mutate in place. This allows optimized mutation when the
		/// shared path can construct the result directly rather than copy-then-mutate.
		/// @param func A callable invoked as @c func(T&) to mutate the value in place when unique.
		/// @param transform A callable invoked as @c transform(const T&) returning a @p T to construct
		/// the new value when shared.
		/// @pre The instance must not be valueless.
		/// @warning Calling @c modify() concurrently on the same instance is a data race and requires
		/// external synchronization. Pass the @c copy_on_write by value to each thread rather than
		/// sharing a reference, the copy-on-write invariant handles the rest.
		/// @warning Storing the reference passed to @p func or @p transform beyond the callback's return
		/// is undefined behaviour. The reference may be invalidated by subsequent operations on this instance.
		template <typename Func, typename Transform>
			requires mclo::invocable_r<void, Func, T&> && mclo::invocable_r<T, Transform, const T&>
		void modify( Func&& func, Transform&& transform )
		{
			DEBUG_ASSERT( !valueless_after_move(), "Modifying copy_on_write that is valueless_after_move" );

			if ( m_ptr->m_counter.load( std::memory_order_acquire ) > 1 )
			{
				storage* tmp =
					create( m_alloc, std::forward<Transform>( transform )( std::as_const( m_ptr->m_value ) ) );
				destroy();
				m_ptr = tmp;
			}
			else
			{
				std::forward<Func>( func )( m_ptr->m_value );
			}
		}

		/// @brief Swaps the contents with @p other.
		/// @warning Swapping instances with unequal, non-propagating allocators is undefined behaviour.
		void swap( copy_on_write& other ) noexcept( alloc_traits::propagate_on_container_swap::value ||
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
					UNREACHABLE( "Cannot swap copy_on_writes with different allocators" );
				}
			}
		}

		/// @brief Swaps two @c copy_on_write instances.
		friend void swap( copy_on_write& lhs, copy_on_write& rhs ) noexcept( noexcept( lhs.swap( rhs ) ) )
		{
			lhs.swap( rhs );
		}

		template <class U, class AA>
		[[nodiscard]] friend bool operator==( const copy_on_write& lhs,
											  const copy_on_write<U, AA>& rhs ) noexcept( noexcept( *lhs == *rhs ) )
		{
			if ( lhs.valueless_after_move() )
			{
				return rhs.valueless_after_move();
			}
			else if ( rhs.valueless_after_move() )
			{
				return false;
			}

			// Early out if identical storage of same type and allocator
			if constexpr ( std::is_same_v<T, U> && std::is_same_v<Allocator, AA> )
			{
				if ( lhs.identical_to( rhs ) )
				{
					return true;
				}
			}

			return *lhs == *rhs;
		}

		template <class U, class AA>
		[[nodiscard]] friend auto operator<=>( const copy_on_write& lhs,
											   const copy_on_write<U, AA>& rhs ) noexcept( noexcept( *lhs <=> *rhs ) )
			-> mclo::synth_three_way_result<T, U>
		{
			if ( lhs.valueless_after_move() || rhs.valueless_after_move() )
			{
				return !lhs.valueless_after_move() <=> !rhs.valueless_after_move();
			}

			// Early out if identical storage of same type and allocator
			if constexpr ( std::is_same_v<T, U> && std::is_same_v<Allocator, AA> )
			{
				if ( lhs.identical_to( rhs ) )
				{
					return std::strong_ordering::equal;
				}
			}

			return mclo::synth_three_way{}( *lhs, *rhs );
		}

		template <class U>
		[[nodiscard]] friend bool operator==( const copy_on_write& lhs,
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
		[[nodiscard]] friend auto operator<=>( const copy_on_write& lhs,
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
		struct storage
		{
			std::atomic<long> m_counter{ 1 };

			union
			{
				char m_dummy{};
				value_type m_value;
			};

			~storage()
			{
			}
		};

		using storage_alloc = typename alloc_traits::template rebind_alloc<storage>;
		using storage_alloc_traits = typename std::allocator_traits<storage_alloc>;

		void destroy() noexcept
		{
			if ( m_ptr )
			{
				const std::size_t old = m_ptr->m_counter.fetch_sub( 1, std::memory_order_release );
				DEBUG_ASSERT( old != 0, "Reference count underflow in copy_on_write" );
				if ( old == 1 ) // Was last reference
				{
					std::atomic_thread_fence( std::memory_order_acquire );
					alloc_traits::destroy( m_alloc, std::addressof( m_ptr->m_value ) );

					storage_alloc alloc( m_alloc );
					storage_alloc_traits::destroy( alloc, m_ptr );
					storage_alloc_traits::deallocate( alloc, m_ptr, 1 );
				}

				m_ptr = nullptr;
			}
		}

		template <typename... Ts>
		[[nodiscard]] static storage* create( allocator_type& allocator, Ts&&... ts )
		{
			storage_alloc alloc( allocator );
			storage* ptr = storage_alloc_traits::allocate( alloc, 1 );
			try
			{
				storage_alloc_traits::construct( alloc, ptr );

				try
				{
					alloc_traits::construct( allocator, std::addressof( ptr->m_value ), std::forward<Ts>( ts )... );
				}
				catch ( ... )
				{
					storage_alloc_traits::destroy( alloc, ptr );
					throw;
				}

				return ptr;
			}
			catch ( ... )
			{
				storage_alloc_traits::deallocate( alloc, ptr, 1 );
				throw;
			}
		}

		storage* m_ptr = nullptr;
		MCLO_NO_UNIQUE_ADDRESS allocator_type m_alloc;
	};

	/// @brief Deduction guide: deduces @c copy_on_write<T> from a value of type @p T.
	template <typename T>
	copy_on_write( T ) -> copy_on_write<T>;

	/// @brief Deduction guide: deduces @c copy_on_write<T, Allocator> from a value and allocator.
	template <typename T, typename Allocator>
	copy_on_write( std::allocator_arg_t, Allocator, T )
		-> copy_on_write<T, typename std::allocator_traits<Allocator>::template rebind_alloc<T>>;

	/// @brief Alias for @c copy_on_write using @c std::pmr::polymorphic_allocator.
	namespace pmr
	{
		template <typename T>
		using copy_on_write = mclo::copy_on_write<T, std::pmr::polymorphic_allocator<T>>;
	}
}

/// @brief Hash specialization for @c mclo::copy_on_write.
/// @details Delegates to the hash of the contained value. A valueless instance returns a fixed value.
template <typename T, typename Allocator>
	requires requires( T a ) { std::hash<T>{}( a ); }
struct std::hash<mclo::copy_on_write<T, Allocator>>
{
	std::size_t operator()( const mclo::copy_on_write<T, Allocator>& value ) const noexcept
	{
		if ( value.valueless_after_move() )
		{
			return 42;
		}
		return std::hash<T>{}( *value );
	}
};
