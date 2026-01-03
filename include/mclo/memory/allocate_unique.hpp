#pragma once

#include "mclo/preprocessor/platform.hpp"

#include <memory>

namespace mclo
{
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

	template <typename T, typename Allocator, typename... Args>
	[[nodiscard]] auto allocate_unique( Allocator& allocator, Args&&... args )
	{
		static_assert( !std::is_array_v<T>, "allocate_unique does not support array types" );
		using alloc = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
		return std::unique_ptr<T, allocation_deleter<alloc>>(
			allocator_new<T>( allocator, std::forward<Args>( args )... ), allocator );
	}
}
