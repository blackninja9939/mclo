#pragma once

#include <memory>

#ifdef __cpp_lib_polymorphic

namespace mclo
{
	using std::polymorphic;
}

#else

#include "mclo/concepts/specialization_of.hpp"
#include "mclo/debug/assert.hpp"
#include "mclo/preprocessor/platform.hpp"

#include <compare>
#include <concepts>
#include <type_traits>

namespace mclo
{
	template <typename T, typename Allocator = std::allocator<T>>
	class polymorphic
	{
		using alloc_traits = std::allocator_traits<Allocator>;
		static_assert( std::is_same_v<T, typename alloc_traits::value_type>, "Allocator::value_type must be T" );
		static_assert( std::is_object_v<T>, "T must be an object type" );
		static_assert( !std::is_array_v<T>, "T must not be an array type" );
		static_assert( !std::is_same_v<T, std::in_place_t>, "T cannot be std::in_place_t" );
		static_assert( !mclo::specialization_of<T, std::in_place_type_t>,
					   "T cannot be a specialization of std::in_place_type_t" );
		static_assert( std::is_same_v<T, std::remove_cv_t<T>>, "T cannot be cv qualified" );

		// todo(mc) the specification allows for using an inline buffer of memory like std::any, maybe I should do that?
		// can we implement the abstract functions as a manual function pointer vtable instead? Is that dispatch faster than the virtual calls?
		// rolling it manually would involve an allocation and virtual calls anyway for a unique_ptr like type wrapping to do clone etc

		struct control_block
		{
			typename alloc_traits::pointer m_ptr = nullptr;

			virtual constexpr ~control_block() = default;
			virtual constexpr void destroy( const Allocator& allocator ) noexcept = 0;
			virtual constexpr control_block* clone( const Allocator& allocator ) const = 0;
			virtual constexpr control_block* move( const Allocator& allocator ) = 0;
		};

		template <typename U>
		struct object_control_block final : control_block
		{
			using value_alloc = typename alloc_traits::template rebind_alloc<U>;
			using value_traits = std::allocator_traits<value_alloc>;

			using control_block_alloc = typename alloc_traits::template rebind_alloc<object_control_block>;
			using control_block_traits = std::allocator_traits<control_block_alloc>;

			union storage
			{
				constexpr storage() noexcept
				{
				}
				constexpr ~storage() noexcept
				{
				}
				U m_object;
			};
			storage m_storage;

			template <typename... Ts>
			constexpr object_control_block( const Allocator& allocator, Ts&&... ts )
			{
				value_alloc alloc( allocator );
				value_traits::construct( alloc, &m_storage.m_object, std::forward<Ts>( ts )... );
				control_block::m_ptr = &m_storage.m_object;
			}

			constexpr void destroy( const Allocator& allocator ) noexcept override
			{
				value_alloc alloc( allocator );
				value_traits::destroy( alloc, &m_storage.m_object );
			}

			constexpr control_block* clone( const Allocator& allocator ) const override
			{
				control_block_alloc alloc( allocator );
				object_control_block* copy = control_block_traits::allocate( alloc, 1 );
				try
				{
					control_block_traits::construct( alloc, copy, allocator, m_storage.m_object );
					return copy;
				}
				catch ( ... )
				{
					control_block_traits::deallocate( alloc, copy, 1 );
					throw;
				}
			}

			constexpr control_block* move( const Allocator& allocator ) override
			{
				control_block_alloc alloc( allocator );
				object_control_block* copy = control_block_traits::allocate( alloc, 1 );
				try
				{
					control_block_traits::construct( alloc, copy, allocator, std::move( m_storage.m_object ) );
					return copy;
				}
				catch ( ... )
				{
					control_block_traits::deallocate( alloc, copy, 1 );
					throw;
				}
			}
		};

		template <typename U>
		static constexpr bool is_convertible_from =
			!std::is_same_v<std::remove_cvref_t<U>, polymorphic> && std::derived_from<std::remove_cvref_t<U>, T> &&
			std::is_constructible_v<std::remove_cvref_t<U>, U> &&
			std::is_copy_constructible_v<std::remove_cvref_t<U>> &&
			!mclo::specialization_of<std::remove_cvref_t<U>, std::in_place_type_t>;

		template <typename U, typename... Ts>
		static constexpr bool is_constructible_from =
			std::is_same_v<std::remove_cvref_t<U>, U> && std::derived_from<U, T> && std::is_constructible_v<U, Ts...> &&
			std::is_copy_constructible_v<U>;

	public:
		using value_type = T;
		using allocator_type = Allocator;
		using pointer = typename alloc_traits::pointer;
		using const_pointer = typename alloc_traits::const_pointer;

		explicit constexpr polymorphic()
			requires( std::is_default_constructible_v<allocator_type> )
			: polymorphic( std::allocator_arg, allocator_type{} )
		{
			static_assert( std::is_default_constructible_v<T>, "T must be default constructible" );
			static_assert( std::is_copy_constructible_v<T>, "T must be copy constructible" );
		}

		explicit constexpr polymorphic( std::allocator_arg_t, const allocator_type& alloc )
			: m_alloc( alloc )
		{
			static_assert( std::is_default_constructible_v<T>, "T must be default constructible" );
			static_assert( std::is_copy_constructible_v<T>, "T must be copy constructible" );
			m_control_block = create<T>( m_alloc );
		}

		constexpr polymorphic( const polymorphic& other )
			: polymorphic(
				  std::allocator_arg, alloc_traits::select_on_container_copy_construction( other.m_alloc ), other )
		{
		}

		constexpr polymorphic( std::allocator_arg_t, const Allocator& alloc, const polymorphic& other )
			: m_alloc( alloc )
		{
			if ( !other.valueless_after_move() )
			{
				m_control_block = other.m_control_block->clone( m_alloc );
			}
		}

		constexpr polymorphic( polymorphic&& other ) noexcept
			: polymorphic( std::allocator_arg, other.m_alloc, std::move( other ) )
		{
		}

		constexpr polymorphic( std::allocator_arg_t,
							   const Allocator& alloc,
							   polymorphic&& other ) noexcept( alloc_traits::is_always_equal::value )
			: m_alloc( alloc )
		{
			using std::swap;
			if constexpr ( alloc_traits::is_always_equal::value )
			{
				swap( m_control_block, other.m_control_block );
			}
			else
			{
				static_assert( sizeof( T ) != 0, "T must be complete" );
				if ( m_alloc == other.m_alloc )
				{
					swap( m_control_block, other.m_control_block );
				}
				else if ( !other.valueless_after_move() )
				{
					m_control_block = other.m_control_block->move( m_alloc );
				}
			}
		}

		template <typename U = T>
		explicit constexpr polymorphic( U&& value )
			requires( is_convertible_from<U> && std::is_default_constructible_v<allocator_type> )
			: polymorphic( std::allocator_arg, allocator_type{}, std::forward<U>( value ) )
		{
		}

		template <typename U = T>
		explicit constexpr polymorphic( std::allocator_arg_t, const Allocator& alloc, U&& value )
			requires( is_convertible_from<U> )
			: m_alloc( alloc )
		{
			m_control_block = create<U>( m_alloc, std::forward<U>( value ) );
		}

		template <typename U, typename... Us>
			requires( is_constructible_from<U, Us...> && std::is_default_constructible_v<allocator_type> )
		explicit constexpr polymorphic( std::in_place_type_t<U>, Us&&... us )
			: polymorphic( std::allocator_arg, allocator_type{}, std::in_place_type<U>, std::forward<Us>( us )... )
		{
		}

		template <typename U, typename... Us>
			requires( is_constructible_from<U, Us...> )
		explicit constexpr polymorphic( std::allocator_arg_t,
										const Allocator& alloc,
										std::in_place_type_t<U>,
										Us&&... us )
			: m_alloc( alloc )
		{
			m_control_block = create<U>( m_alloc, std::forward<Us>( us )... );
		}

		template <typename U, typename I, typename... Us>
			requires( is_constructible_from<U, std::initializer_list<I>&, Us...> &&
					  std::is_default_constructible_v<allocator_type> )
		explicit constexpr polymorphic( std::in_place_type_t<U>, std::initializer_list<I> init_list, Us&&... us )
			: polymorphic(
				  std::allocator_arg, allocator_type{}, std::in_place_type<U>, init_list, std::forward<Us>( us )... )
		{
		}

		template <typename U, typename I, typename... Us>
			requires( is_constructible_from<U, std::initializer_list<I>&, Us...> )
		explicit constexpr polymorphic( std::allocator_arg_t,
										const Allocator& alloc,
										std::in_place_type_t<U>,
										std::initializer_list<I> init_list,
										Us&&... us )
			: m_alloc( alloc )
		{
			m_control_block = create<U>( m_alloc, init_list, std::forward<Us>( us )... );
		}

		constexpr ~polymorphic()
		{
			static_assert( sizeof( T ) != 0, "T must be complete" );
			destroy();
		}

		constexpr polymorphic& operator=( const polymorphic& other )
		{
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
				control_block* tmp = other.m_control_block->clone( update_alloc ? other.m_alloc : m_alloc );
				destroy();
				m_control_block = tmp;
			}

			if ( update_alloc )
			{
				m_alloc = other.m_alloc;
			}

			return *this;
		}

		constexpr polymorphic& operator=( polymorphic&& other ) noexcept(
			alloc_traits::propagate_on_container_move_assignment::value || alloc_traits::is_always_equal::value )
		{
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
					swap( m_control_block, other.m_control_block );
					other.destroy();
				}
				else
				{
					control_block* tmp = other.m_control_block->move( update_alloc ? other.m_alloc : m_alloc );
					destroy();
					m_control_block = tmp;
				}
			}

			if ( update_alloc )
			{
				m_alloc = other.m_alloc;
			}

			return *this;
		}

		[[nodiscard]] constexpr const T& operator*() const& MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !valueless_after_move(), "Dereferencing indirect that is valueless_after_move" );
			return *m_control_block->m_ptr;
		}

		[[nodiscard]] constexpr T& operator*() & MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !valueless_after_move(), "Dereferencing indirect that is valueless_after_move" );
			return *m_control_block->m_ptr;
		}

		[[nodiscard]] constexpr const T&& operator*() const&& MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !valueless_after_move(), "Dereferencing indirect that is valueless_after_move" );
			return std::move( **this );
		}

		[[nodiscard]] constexpr T&& operator*() && MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !valueless_after_move(), "Dereferencing indirect that is valueless_after_move" );
			return std::move( **this );
		}

		[[nodiscard]] constexpr const_pointer operator->() const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !valueless_after_move(), "Getting pointer for indirect that is valueless_after_move" );
			return m_control_block->m_ptr;
		}

		[[nodiscard]] constexpr pointer operator->() MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !valueless_after_move(), "Getting pointer for indirect that is valueless_after_move" );
			return m_control_block->m_ptr;
		}

		[[nodiscard]] constexpr bool valueless_after_move() const noexcept
		{
			return m_control_block == nullptr;
		}

		[[nodiscard]] constexpr allocator_type get_allocator() const noexcept
		{
			return m_alloc;
		}

		constexpr void swap( polymorphic& other ) noexcept( alloc_traits::propagate_on_container_swap::value ||
															alloc_traits::is_always_equal::value )
		{
			using std::swap;

			if constexpr ( alloc_traits::propagate_on_container_swap::value )
			{
				swap( m_alloc, other.m_alloc );
				swap( m_control_block, other.m_control_block );
			}
			else
			{
				if ( m_alloc == other.m_alloc )
				{
					swap( m_control_block, other.m_control_block );
				}
				else
				{
					UNREACHABLE( "Cannot swap polymorphics with different allocators" );
				}
			}
		}

		friend constexpr void swap( polymorphic& lhs, polymorphic& rhs ) noexcept( noexcept( lhs.swap( rhs ) ) )
		{
			lhs.swap( rhs );
		}

	private:
		constexpr void destroy() noexcept
		{
			if ( m_control_block )
			{
				m_control_block->destroy( m_alloc );
				m_control_block = nullptr;
			}
		}

		template <typename U, typename... Ts>
		[[nodiscard]] constexpr static object_control_block<U>* create( const allocator_type& allocator, Ts&&... ts )
		{
			using control_block_alloc = typename alloc_traits::template rebind_alloc<object_control_block<U>>;
			using control_block_traits = std::allocator_traits<control_block_alloc>;

			control_block_alloc alloc( allocator );
			object_control_block<U>* ptr = control_block_traits::allocate( alloc, 1 );

			try
			{
				control_block_traits::construct( alloc, ptr, allocator, std::forward<Ts>( ts )... );
				return ptr;
			}
			catch ( ... )
			{
				control_block_traits::deallocate( alloc, ptr, 1 );
				throw;
			}
		}

		control_block* m_control_block = nullptr;
		MCLO_NO_UNIQUE_ADDRESS allocator_type m_alloc;
	};
}

#endif
