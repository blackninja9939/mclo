#pragma once

#include <memory>

namespace mclo
{
	/// @brief Allocates and constructs a single @p T using an allocator.
	/// @details Rebinds @p allocator to @p T, allocates storage for one object and constructs it from @p args. If
	/// construction throws the storage is deallocated and the exception rethrown.
	/// @tparam T The type to allocate and construct.
	/// @tparam Allocator The allocator type, rebound to @p T internally.
	/// @tparam Args The constructor argument types.
	/// @param allocator The allocator to allocate and construct with.
	/// @param args The arguments forwarded to the constructor of @p T.
	/// @return The allocator's pointer to the constructed object.
	template <typename T, typename Allocator, typename... Args>
	[[nodiscard]] auto allocator_new( Allocator& allocator, Args&&... args )
	{
		using alloc = std::allocator_traits<Allocator>::template rebind_alloc<T>;
		using traits = std::allocator_traits<alloc>;

		alloc alloc_instance{ allocator };
		auto ptr = traits::allocate( alloc_instance, 1 );

		try
		{
			traits::construct( alloc_instance, std::to_address( ptr ), std::forward<Args>( args )... );
			return ptr;
		}
		catch ( ... )
		{
			traits::deallocate( alloc_instance, ptr, 1 );
			throw;
		}
	}

	/// @brief Destroys and deallocates an object previously created with @ref allocator_new.
	/// @tparam Allocator The allocator type, rebound to the pointee type internally.
	/// @tparam P The pointer type to the object to destroy.
	/// @param allocator The allocator that owns the storage.
	/// @param ptr Pointer to the object to destroy and deallocate.
	template <typename Allocator, typename P>
	void allocator_delete( Allocator& allocator, P ptr )
	{
		using Elem = typename std::pointer_traits<P>::element_type;
		using alloc = std::allocator_traits<Allocator>::template rebind_alloc<Elem>;
		using traits = std::allocator_traits<alloc>;

		alloc alloc_instance{ allocator };
		traits::destroy( alloc_instance, std::to_address( ptr ) );
		traits::deallocate( alloc_instance, ptr, 1 );
	}

	/// @brief Deleter that destroys an object via an allocator, for use with @c std::unique_ptr.
	/// @details Inherits from the allocator to benefit from empty base optimization and calls @ref allocator_delete
	/// on invocation.
	/// @tparam Allocator The allocator type used to destroy and deallocate the object.
	template <typename Allocator>
	struct allocation_deleter : public Allocator
	{
		using pointer = typename std::allocator_traits<Allocator>::pointer;

		allocation_deleter( const Allocator& alloc ) noexcept
			: Allocator( alloc )
		{
		}

		template <typename U>
		allocation_deleter( const allocation_deleter<U>& other ) noexcept
			: Allocator( other )
		{
		}

		void operator()( pointer ptr )
		{
			allocator_delete( static_cast<Allocator&>( *this ), ptr );
		}
	};

	/// @brief Creates a @c std::unique_ptr owning a @p T allocated and constructed via an allocator.
	/// @details Like @c std::make_unique but uses @p allocator for allocation and construction, returning a unique
	/// pointer whose deleter is an @ref allocation_deleter. Array types are not supported.
	/// @tparam T The type to allocate and construct.
	/// @tparam Allocator The allocator type, rebound to @p T internally.
	/// @tparam Args The constructor argument types.
	/// @param allocator The allocator to allocate and construct with.
	/// @param args The arguments forwarded to the constructor of @p T.
	/// @return A @c std::unique_ptr owning the constructed object with an allocator-aware deleter.
	template <typename T, typename Allocator, typename... Args>
	[[nodiscard]] auto allocate_unique( Allocator& allocator, Args&&... args )
	{
		static_assert( !std::is_array_v<T>, "allocate_unique does not support array types" );
		using alloc = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
		return std::unique_ptr<T, allocation_deleter<alloc>>(
			allocator_new<T>( allocator, std::forward<Args>( args )... ), allocator );
	}
}
