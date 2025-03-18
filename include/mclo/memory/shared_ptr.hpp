#pragma once

#include "mclo/preprocessor/platform.hpp"

#include <atomic>
#include <memory>
#include <typeinfo>
#include <utility>

namespace mclo
{
	namespace detail
	{
		class MCLO_NO_VTABLE control_block_base
		{
		public:
			virtual ~control_block_base() = default;

			virtual void destroy_object() noexcept = 0;
			virtual void deallocate_this() noexcept = 0;

			// todo(mc) this should not exist, its used for weak_ptr only, move it so that weak_ptr knows its actual pointer like shared_ptr
			// and constructs from that
			[[nodiscard]] virtual void* get_ptr() const noexcept = 0;

			[[nodiscard]] virtual void* get_deleter(
				[[maybe_unused]] const std::type_info& deleter_type ) const noexcept
			{
				return nullptr;
			}

			void decrement_weak() noexcept
			{
				const counter_t last_count = weak_counter.fetch_add( -1, std::memory_order_acq_rel );
				if ( last_count == 1 ) // We were last weak reference
				{
					deallocate_this();
				}
			}

			void decrement_owner() noexcept
			{
				const counter_t last_count = owner_counter.fetch_add( -1, std::memory_order_acq_rel );
				if ( last_count == 1 ) // We were last owner
				{
					destroy_object();
					decrement_weak();
				}
			}

			void add_weak() noexcept
			{
				weak_counter.fetch_add( 1, std::memory_order_relaxed );
			}

			void add_owner() noexcept
			{
				owner_counter.fetch_add( 1, std::memory_order_relaxed );
			}

			[[nodiscard]] void* try_add_owner() noexcept
			{
				counter_t expected = owner_counter.load( std::memory_order_relaxed );

				while ( expected != 0 )
				{
					if ( owner_counter.compare_exchange_weak( expected, expected + 1 ) )
					{
						return get_ptr();
					}
				}

				return nullptr;
			}

			[[nodiscard]] long get_owner_count() const noexcept
			{
				return static_cast<long>( owner_counter.load() );
			}

		private:
			using counter_t = long;
			using atomic_counter_t = std::atomic<counter_t>;
			
			atomic_counter_t owner_counter{ 1 };

			// weak is always 1 if there is any active so as to avoid race conditions with the active counter
			atomic_counter_t weak_counter{ 1 };
		};

		template <typename T>
		class control_block_default final : public control_block_base
		{
		public:
			explicit control_block_default( T* ptr ) noexcept
				: ptr( ptr )
			{
			}

			void destroy_object() noexcept override
			{
				delete ptr;
			}
			void deallocate_this() noexcept override
			{
				delete this;
			}

			[[nodiscard]] void* get_ptr() const noexcept override
			{
				return ptr;
			}

		private:
			T* ptr;
		};

		template <typename T>
		class control_block_default<T[]> final : public control_block_base
		{
		public:
			explicit control_block_default( T* ptr ) noexcept
				: ptr( ptr )
			{
			}

			void destroy_object() noexcept override
			{
				delete[] ptr;
			}
			void deallocate_this() noexcept override
			{
				delete this;
			}

			[[nodiscard]] void* get_ptr() const noexcept override
			{
				return ptr;
			}

		private:
			T* ptr;
		};

		template <typename T>
		class control_block_inline_object final : public control_block_base
		{
		public:
			template <typename... Args>
			explicit control_block_inline_object( Args&&... args ) noexcept
			{
				std::construct_at( get_object(), std::forward<Args>( args )... );
			}

			void destroy_object() noexcept override
			{
				std::destroy_at( get_object() );
			}
			void deallocate_this() noexcept override
			{
				delete this;
			}

			[[nodiscard]] void* get_ptr() const noexcept override
			{
				return get_object();
			}

			[[nodiscard]] MCLO_FORCE_INLINE T* get_object() const noexcept
			{
				return const_cast<T*>( reinterpret_cast<const T*>( buffer ) );
			}

		private:
			alignas( T ) unsigned char buffer[ sizeof( T ) ];
		};

		template <typename T, typename TDeleter>
		class control_block_deleter final : public control_block_base
		{
		public:
			explicit control_block_deleter( T* ptr, TDeleter deleter ) noexcept
				: ptr( ptr )
				, deleter( std::move( deleter ) )
			{
			}

			void destroy_object() noexcept override
			{
				deleter( ptr );
			}
			void deallocate_this() noexcept override
			{
				delete this;
			}

			[[nodiscard]] void* get_ptr() const noexcept override
			{
				return ptr;
			}

			[[nodiscard]] void* get_deleter( const std::type_info& deleter_type ) const noexcept override
			{
				if ( deleter_type == typeid( TDeleter ) )
				{
					return const_cast<TDeleter*>( std::addressof( deleter ) );
				}
				return nullptr;
			}

		private:
			T* ptr;
			MCLO_NO_UNIQUE_ADDRESS TDeleter deleter;
		};

		template <typename T, typename TDeleter, typename TAllocator>
		class control_block_allocator_deleter final : public control_block_base
		{
		public:
			using this_allocator_t =
				std::allocator_traits<TAllocator>::template rebind_alloc<control_block_allocator_deleter>;
			using this_alloc_traits = std::allocator_traits<this_allocator_t>;

			explicit control_block_allocator_deleter( T* ptr, TDeleter deleter, const TAllocator& allocator ) noexcept
				: ptr( ptr )
				, deleter( std::move( deleter ) )
				, allocator( allocator )
			{
			}

			void destroy_object() noexcept override
			{
				deleter( ptr );
			}
			void deallocate_this() noexcept override
			{
				this_alloc_traits::destroy( allocator, this );
				this_alloc_traits::deallocate( allocator, this, 1 );
			}

			[[nodiscard]] void* get_ptr() const noexcept override
			{
				return ptr;
			}

			[[nodiscard]] void* get_deleter( const std::type_info& deleter_type ) const noexcept override
			{
				if ( deleter_type == typeid( TDeleter ) )
				{
					return const_cast<TDeleter*>( std::addressof( deleter ) );
				}
				return nullptr;
			}

		private:
			T* ptr;
			MCLO_NO_UNIQUE_ADDRESS TDeleter deleter;
			MCLO_NO_UNIQUE_ADDRESS this_allocator_t allocator;
		};

		// When constructing a shared pointer if the allocation of the control block throws the pointer to be owned
		// still needs to get deleted, so we wrap it in a simple RAII owner that handles that in the destructor
		template <typename T>
		struct [[nodiscard]] temporary_unique_owner
		{
			explicit temporary_unique_owner( T* ptr )
				: ptr( ptr )
			{
			}
			~temporary_unique_owner()
			{
				delete ptr;
			}

			temporary_unique_owner( const temporary_unique_owner& other ) = delete;
			temporary_unique_owner& operator=( const temporary_unique_owner& other ) = delete;

			T* release()
			{
				return std::exchange( ptr, nullptr );
			}

			T* ptr;
		};

		template <typename T>
		struct [[nodiscard]] temporary_unique_owner<T[]>
		{
			explicit temporary_unique_owner( T* ptr )
				: ptr( ptr )
			{
			}
			~temporary_unique_owner()
			{
				delete[] ptr;
			}

			temporary_unique_owner( const temporary_unique_owner& other ) = delete;
			temporary_unique_owner& operator=( const temporary_unique_owner& other ) = delete;

			T* release()
			{
				return std::exchange( ptr, nullptr );
			}

			T* ptr;
		};

		template <typename T, typename TDeleter>
		struct [[nodiscard]] temporary_unique_owner_deleter
		{
			explicit temporary_unique_owner_deleter( T* ptr, TDeleter& deleter )
				: ptr( ptr )
				, deleter( deleter )
			{
			}
			~temporary_unique_owner_deleter()
			{
				if ( ptr )
				{
					deleter( ptr );
				}
			}

			temporary_unique_owner_deleter( const temporary_unique_owner_deleter& other ) = delete;
			temporary_unique_owner_deleter& operator=( const temporary_unique_owner_deleter& other ) = delete;

			T* release()
			{
				return std::exchange( ptr, nullptr );
			}

			T* ptr;
			TDeleter& deleter;
		};

		// The allocator can throw in its construction of the object so we need to wrap the actual allocation
		// in this RAII type to deallocate it
		template <typename TAllocator>
		struct [[nodiscard]] temporary_allocator_owner
		{
			using traits = std::allocator_traits<TAllocator>;
			using pointer = typename traits::pointer;

			explicit temporary_allocator_owner( TAllocator& allocator )
				: allocator( allocator )
			{
			}
			~temporary_allocator_owner()
			{
				if ( ptr )
				{
					traits::deallocate( allocator, ptr, 1 );
				}
			}

			temporary_allocator_owner( const temporary_allocator_owner& other ) = delete;
			temporary_allocator_owner& operator=( const temporary_allocator_owner& other ) = delete;

			// Allocate memory for a new object
			void allocate()
			{
				// If allocate throws then we can double delete ptr unless we null it out
				ptr = nullptr;
				ptr = traits::allocate( allocator, 1 );
			}

			// Release the newly allocated pointer to its owner, should be fully constructed in by that point
			[[nodiscard]] pointer release()
			{
				return std::exchange( ptr, nullptr );
			}

			TAllocator& allocator;
			pointer ptr{};
		};

		template <typename T, typename = void>
		constexpr bool can_delete_scalar = false;

		template <typename T>
		constexpr bool can_delete_scalar<T, std::void_t<decltype( delete std::declval<T*>() )>> = true;

		template <typename T, typename = void>
		constexpr bool can_delete_array = false;

		template <typename T>
		constexpr bool can_delete_array<T, std::void_t<decltype( delete[] std::declval<T*>() )>> = true;

		template <typename T>
		constexpr bool can_delete = std::is_array_v<T> ? can_delete_array<T> : can_delete_scalar<T>;

		template <typename T, typename TDeleter>
		constexpr bool can_delete_deleter = std::is_invocable_v<TDeleter&, T*&>;

		// Standard defines convertible as convertible pointers or arrays with same extents
		template <typename From, typename To>
		constexpr bool is_convertible_shared_ptr = std::is_convertible_v<From*, To*>;

		template <typename From, typename To>
		constexpr bool is_convertible_shared_ptr<From, To[]> = std::is_convertible_v<From ( * )[], To ( * )[]>;

		template <typename From, typename To, std::size_t Extents>
		constexpr bool is_convertible_shared_ptr<From, To[ Extents ]> =
			std::is_convertible_v<From ( * )[ Extents ], To ( * )[ Extents ]>;

		// Standard defines compatible as convertible pointers or arrays with cv qualifiers
		template <typename From, typename To>
		constexpr bool is_compatible_shared_ptr = std::is_convertible_v<From*, To*>;

		template <typename From, std::size_t Extents>
		constexpr bool is_compatible_shared_ptr<From[ Extents ], From[]> = true;

		template <typename From, std::size_t Extents>
		constexpr bool is_compatible_shared_ptr<From[ Extents ], const From[]> = true;

		template <typename From, std::size_t Extents>
		constexpr bool is_compatible_shared_ptr<From[ Extents ], volatile From[]> = true;

		template <typename From, std::size_t Extents>
		constexpr bool is_compatible_shared_ptr<From[ Extents ], const volatile From[]> = true;

		template <typename T>
		concept non_array = !std::is_array_v<T>;
	}

	template <typename T>
	class shared_ptr;

	template <typename T>
	class weak_ptr
	{
	private:
		template <typename U>
		friend class weak_ptr;

		template <typename U>
		friend class shared_ptr;

	public:
		constexpr weak_ptr() noexcept = default;

		weak_ptr( weak_ptr&& other ) noexcept
			: control_block( std::exchange( other.control_block, nullptr ) )
		{
		}

		template <typename U>
			requires detail::is_compatible_shared_ptr<U, T>
		weak_ptr( weak_ptr<U>&& other ) noexcept
			: control_block( std::exchange( other.control_block, nullptr ) )
		{
		}

		weak_ptr( const weak_ptr& other ) noexcept
		{
			if ( other.control_block )
			{
				other.control_block->add_weak();
				control_block = other.control_block;
			}
		}

		template <typename U>
			requires detail::is_compatible_shared_ptr<U, T>
		weak_ptr( const weak_ptr& other ) noexcept
		{
			if ( other.control_block )
			{
				other.control_block->add_weak();
				control_block = other.control_block;
			}
		}

		template <typename U>
			requires detail::is_compatible_shared_ptr<U, T>
		weak_ptr( const shared_ptr<U>& other ) noexcept
		{
			if ( other.control_block )
			{
				other.control_block->add_weak();
				control_block = other.control_block;
			}
		}

		weak_ptr& operator=( weak_ptr&& other ) noexcept
		{
			weak_ptr( std::move( other ) ).swap( *this );
			return *this;
		}

		template <typename U>
			requires detail::is_compatible_shared_ptr<U, T>
		weak_ptr& operator=( weak_ptr<U>&& other ) noexcept
		{
			weak_ptr( std::move( other ) ).swap( *this );
			return *this;
		}

		weak_ptr& operator=( const weak_ptr& other ) noexcept
		{
			weak_ptr( other ).swap( *this );
			return *this;
		}

		template <typename U>
			requires detail::is_compatible_shared_ptr<U, T>
		weak_ptr& operator=( const weak_ptr& other ) noexcept
		{
			weak_ptr( other ).swap( *this );
			return *this;
		}

		template <typename U>
			requires detail::is_compatible_shared_ptr<U, T>
		weak_ptr& operator=( const shared_ptr<U>& other ) noexcept
		{
			weak_ptr( other ).swap( *this );
			return *this;
		}

		~weak_ptr()
		{
			if ( control_block )
			{
				control_block->decrement_weak();
			}
		}

		void swap( weak_ptr& other ) noexcept
		{
			std::swap( control_block, other.control_block );
		}

		void reset() noexcept
		{
			weak_ptr().swap( *this );
		}

		[[nodiscard]] long use_count() const noexcept
		{
			return control_block ? control_block->get_owner_count() : 0;
		}

		[[nodiscard]] bool expired() const noexcept
		{
			return this->use_count() == 0;
		}

		[[nodiscard]] shared_ptr<T> lock() const noexcept
		{
			if ( !control_block )
			{
				return {};
			}

			T* ptr = static_cast<T*>( control_block->try_add_owner() );
			if ( !ptr )
			{
				return {};
			}

			shared_ptr<T> shared;
			shared.set_data( ptr, control_block );
			return shared;
		}

		template <typename U>
		[[nodiscard]] bool owner_before( const shared_ptr<U>& other ) const noexcept
		{
			return control_block < other.control_block;
		}
		template <typename U>
		[[nodiscard]] bool owner_before( const weak_ptr<U>& other ) const noexcept
		{
			return control_block < other.control_block;
		}

	private:
		detail::control_block_base* control_block = nullptr;
	};

	template <typename T>
	weak_ptr( shared_ptr<T> ) -> weak_ptr<T>;

	template <typename T>
	void swap( weak_ptr<T>& lhs, weak_ptr<T>& rhs )
	{
		lhs.swap( rhs );
	}

	template <typename T>
	class enabled_shared_from_this
	{
	public:
		constexpr enabled_shared_from_this() noexcept = default;
		constexpr enabled_shared_from_this( const enabled_shared_from_this& other ) noexcept
		{
		}

		[[nodiscard]] shared_ptr<T> shared_from_this()
		{
			return shared_ptr<T>( weak_this );
		}
		[[nodiscard]] shared_ptr<const T> shared_from_this() const
		{
			return shared_ptr<const T>( weak_this );
		}

		[[nodiscard]] weak_ptr<T> weak_from_this()
		{
			return weak_this;
		}
		[[nodiscard]] weak_ptr<const T> weak_from_this() const
		{
			return weak_this;
		}

	protected:
		enabled_shared_from_this& operator=( const enabled_shared_from_this& other )
		{
			return *this;
		}

	private:
		template <typename U>
		friend class shared_ptr;

		mutable weak_ptr<T> weak_this;
	};

	class bad_weak_ptr : public std::exception
	{
	public:
		bad_weak_ptr() noexcept = default;
		bad_weak_ptr( const bad_weak_ptr& other ) noexcept = default;

		[[nodiscard]] const char* what() const noexcept override
		{
			return "Bad weak_ptr";
		}
	};

	template <typename T>
	class shared_ptr
	{
	private:
		template <typename U>
		friend class shared_ptr;

		template <typename U>
		friend class weak_ptr;

		template <detail::non_array U, typename... Args>
		friend shared_ptr<U> make_shared( Args&&... args );

		template <typename TDeleter, typename T>
		friend TDeleter* get_deleter( const shared_ptr<T>& shared ) noexcept;

	public:
		using element_type = std::remove_extent_t<T>;
		using weak_type = weak_ptr<T>;

		constexpr shared_ptr() noexcept = default;
		constexpr shared_ptr( std::nullptr_t ) noexcept
		{
		}

		template <typename U>
			requires detail::is_convertible_shared_ptr<U, T> && detail::can_delete<U>
		explicit shared_ptr( U* new_ptr )
		{
			using new_type = std::conditional_t<std::is_array_v<T>, U[], U>;
			detail::temporary_unique_owner<new_type> temp_owner( new_ptr );
			set_data( temp_owner.ptr, new detail::control_block_default<new_type>( new_ptr ) );
			temp_owner.release();
		}

		template <typename U, typename TDeleter>
			requires detail::is_convertible_shared_ptr<U, T> && detail::can_delete_deleter<U, TDeleter> &&
					 std::is_move_constructible_v<TDeleter>
		shared_ptr( U* new_ptr, TDeleter deleter )
		{
			detail::temporary_unique_owner_deleter<U, TDeleter> temp_owner( new_ptr, deleter );
			set_data_no_shared_from_this(
				new_ptr, new detail::control_block_deleter<U, TDeleter>( new_ptr, std::move( deleter ) ) );
			temp_owner.release();
		}

		template <typename U, typename TDeleter, typename TAllocator>
			requires detail::is_convertible_shared_ptr<U, T> && detail::can_delete_deleter<U, TDeleter> &&
					 std::is_move_constructible_v<TDeleter>
		shared_ptr( U* new_ptr, TDeleter deleter, TAllocator allocator )
		{
			using control_block_t = detail::control_block_allocator_deleter<U, TDeleter, TAllocator>;

			detail::temporary_unique_owner_deleter<U, TDeleter> temp_owner( new_ptr, deleter );

			typename control_block_t::this_allocator_t block_allocator( allocator );
			detail::temporary_allocator_owner<typename control_block_t::this_allocator_t> allocator_owner(
				block_allocator );
			allocator_owner.allocate();

			using allocator_traits_t = control_block_t::this_alloc_traits;
			allocator_traits_t::construct(
				block_allocator, allocator_owner.ptr, new_ptr, std::move( deleter ), allocator );
			set_data_no_shared_from_this( temp_owner.release(), allocator_owner.release() );
		}

		template <typename U>
		shared_ptr( const shared_ptr<U>& other, element_type* new_ptr ) noexcept
		{
			other.control_block->add_owner();
			set_data_no_shared_from_this( new_ptr, other.control_block );
		}

		template <typename U>
		shared_ptr( shared_ptr<U>&& other, element_type* new_ptr ) noexcept
		{
			other.ptr = nullptr;
			set_data_no_shared_from_this( new_ptr, std::exchange( other.control_block, nullptr ) );
		}

		shared_ptr( const shared_ptr& other ) noexcept
		{
			other.control_block->add_owner();
			set_data_no_shared_from_this( other.ptr, other.control_block );
		}

		template <typename U>
			requires detail::is_compatible_shared_ptr<U, T>
		shared_ptr( const shared_ptr<U>& other ) noexcept
		{
			other.control_block->add_owner();
			set_data_no_shared_from_this( other.ptr, other.control_block );
		}

		shared_ptr( shared_ptr&& other ) noexcept
		{
			set_data_no_shared_from_this( std::exchange( other.ptr, nullptr ),
										  std::exchange( other.control_block, nullptr ) );
		}

		template <typename U>
			requires detail::is_compatible_shared_ptr<U, T>
		shared_ptr( shared_ptr<U>&& other ) noexcept
		{
			set_data_no_shared_from_this( std::exchange( other.ptr, nullptr ),
										  other.std::exchange( other.control_block, nullptr ) );
		}

		template <typename U>
			requires detail::is_compatible_shared_ptr<U, T>
		shared_ptr( const weak_ptr<U>& other )
		{
			if ( !other.control_block )
			{
				throw bad_weak_ptr();
			}

			T* owned_ptr = static_cast<T*>( other.control_block->try_add_owner() );
			if ( !owned_ptr )
			{
				throw bad_weak_ptr();
			}

			set_data_no_shared_from_this( owned_ptr, other.control_block );
		}

		template <typename U, typename TDeleter>
			requires detail::is_compatible_shared_ptr<U, T> &&
					 std::is_convertible_v<typename unique_ptr<U, TDeleter>::pointer, element_type*>
		shared_ptr( unique_ptr<U, TDeleter>&& other ) noexcept
		{
			using pointer_t = typename unique_ptr<U, TDeleter>::pointer;
			using element_type_t = typename unique_ptr<U, TDeleter>::element_type;
			using raw_pointer_t = element_type_t*;
			using deleter_t = std::
				conditional_t<std::is_reference_v<TDeleter>, decltype( std::ref( other.get_deleter() ) ), TDeleter>;

			pointer_t new_ptr = other.get();

			if ( new_ptr )
			{
				set_data( new_ptr,
						  new detail::control_block_deleter<element_type_t, deleter_t>(
							  new_ptr, std::forward<TDeleter>( other.get_deleter() ) ) );
				MCLO_IGNORE_NODISCARD( other.release() );
			}
		}

		shared_ptr& operator=( const shared_ptr& other ) noexcept
		{
			shared_ptr( other ).swap( *this );
			return *this;
		}

		template <typename U>
			requires detail::is_compatible_shared_ptr<U, T>
		shared_ptr& operator=( const shared_ptr<U>& other ) noexcept
		{
			shared_ptr( other ).swap( *this );
			return *this;
		}

		shared_ptr& operator=( shared_ptr&& other ) noexcept
		{
			shared_ptr( std::move( other ) ).swap( *this );
			return *this;
		}

		template <typename U>
			requires detail::is_compatible_shared_ptr<U, T>
		shared_ptr& operator=( shared_ptr<U>&& other ) noexcept
		{
			shared_ptr( std::move( other ) ).swap( *this );
			return *this;
		}

		template <typename U, typename TDeleter>
			requires detail::is_compatible_shared_ptr<U, T> &&
					 std::is_convertible_v<typename unique_ptr<U, TDeleter>::pointer, element_type*>
		shared_ptr& operator=( unique_ptr<U, TDeleter>&& other ) noexcept
		{
			shared_ptr( std::move( other ) ).swap( *this );
			return *this;
		}

		~shared_ptr()
		{
			if ( control_block )
			{
				control_block->decrement_owner();
			}
		}

		void reset() noexcept
		{
			shared_ptr().swap( *this );
		}

		template <typename U>
			requires detail::is_convertible_shared_ptr<U, T> && detail::can_delete<U>
		void reset( U* new_ptr )
		{
			shared_ptr( new_ptr ).swap( *this );
		}

		template <typename U, typename TDeleter>
			requires detail::is_convertible_shared_ptr<U, T> && detail::can_delete_deleter<U, TDeleter> &&
					 std::is_move_constructible_v<TDeleter>
		void reset( U* new_ptr, TDeleter deleter )
		{
			shared_ptr( new_ptr, std::move( deleter ) ).swap( *this );
		}

		template <typename U, typename TDeleter, typename TAllocator>
			requires detail::is_convertible_shared_ptr<U, T> && detail::can_delete_deleter<U, TDeleter> &&
					 std::is_move_constructible_v<TDeleter>
		void reset( U* new_ptr, TDeleter deleter, TAllocator allocator )
		{
			shared_ptr( new_ptr, std::move( deleter ), allocator ).swap( *this );
		}

		void swap( shared_ptr& other ) noexcept
		{
			std::swap( ptr, other.ptr );
			std::swap( control_block, other.control_block );
		}

		[[nodiscard]] element_type* get() const noexcept
		{
			return ptr;
		}

		[[nodiscard]] T& operator*() const noexcept
			requires( !std::is_void_v<T> && !std::is_array_v<T> )
		{
			return *ptr;
		}

		[[nodiscard]] T* operator->() const noexcept
			requires( !std::is_array_v<T> )
		{
			return ptr;
		}

		[[nodiscard]] element_type& operator[]( const std::ptrdiff_t index ) const noexcept
			requires std::is_array_v<T>
		{
			return ptr[ index ];
		}

		[[nodiscard]] explicit operator bool() const noexcept
		{
			return ptr != nullptr;
		}

		[[nodiscard]] long use_count() const noexcept
		{
			return control_block ? control_block->get_owner_count() : 0;
		}

		template <typename U>
		[[nodiscard]] bool owner_before( const shared_ptr<U>& other ) const noexcept
		{
			return control_block < other.control_block;
		}
		template <typename U>
		[[nodiscard]] bool owner_before( const weak_ptr<U>& other ) const noexcept
		{
			return control_block < other.control_block;
		}

	private:
		template <typename U>
		void set_data( U* const new_ptr, detail::control_block_base* const new_control_block ) noexcept
		{
			control_block = new_control_block;
			ptr = new_ptr;

			if constexpr ( !std::is_array_v<U> && !std::is_volatile_v<U> &&
						   std::derived_from<U, enabled_shared_from_this<U>> )
			{
				if ( new_ptr && new_ptr->weak_this.expired() )
				{
					using weak_this_type = std::remove_cv_t<U>;
					new_ptr->weak_this = shared_ptr<weak_this_type>( *this, const_cast<weak_this_type*>( new_ptr ) );
				}
			}
		}

		template <typename U>
		void set_data_no_shared_from_this( U* const new_ptr,
										   detail::control_block_base* const new_control_block ) noexcept
		{
			ptr = new_ptr;
			control_block = new_control_block;
		}

		void set_data( std::nullptr_t, detail::control_block_base* const new_control_block ) noexcept
		{
			ptr = nullptr;
			control_block = new_control_block;
		}

		element_type* ptr = nullptr;
		detail::control_block_base* control_block = nullptr;
	};

	template <typename T>
	shared_ptr( weak_ptr<T> ) -> shared_ptr<T>;

	template <typename T, typename TDeleter>
	shared_ptr( unique_ptr<T, TDeleter> ) -> shared_ptr<T>;

	template <typename T, typename U>
	[[nodiscard]] bool operator==( const shared_ptr<T>& lhs, const shared_ptr<U>& rhs ) noexcept
	{
		return lhs.get() == rhs.get();
	}

	template <typename T, typename U>
	[[nodiscard]] std::strong_ordering operator<=>( const shared_ptr<T>& lhs, const shared_ptr<U>& rhs ) noexcept
	{
		return std::compare_three_way{}( lhs.get(), rhs.get() );
	}

	template <typename T>
	[[nodiscard]] bool operator==( const shared_ptr<T>& lhs, const std::nullptr_t ) noexcept
	{
		return !lhs;
	}

	template <typename T>
	[[nodiscard]] std::strong_ordering operator<=>( const shared_ptr<T>& lhs, const std::nullptr_t ) noexcept
	{
		return std::compare_three_way{}( lhs.get(), static_cast<shared_ptr<T>::element_type*>( nullptr ) );
	}

	template <typename T>
	void swap( shared_ptr<T>& lhs, shared_ptr<T>& rhs )
	{
		lhs.swap( rhs );
	}

	template <detail::non_array T, typename... Args>
	[[nodiscard]] shared_ptr<T> make_shared( Args&&... args )
	{
		const auto control_block = new detail::control_block_inline_object<T>( std::forward<Args>( args )... );
		shared_ptr<T> shared;
		shared.set_data( control_block->get_object(), control_block );
		return shared;
	}

	template <typename T, typename U>
	[[nodiscard]] shared_ptr<T> static_pointer_cast( const shared_ptr<U>& shared ) noexcept
	{
		auto ptr = static_cast<typename shared_ptr<T>::element_type*>( shared.get() );
		return shared_ptr<T>( shared, ptr );
	}

	template <typename T, typename U>
	[[nodiscard]] shared_ptr<T> dynamic_pointer_cast( const shared_ptr<U>& shared ) noexcept
	{
		auto ptr = dynamic_cast<typename shared_ptr<T>::element_type*>( shared.get() );
		if ( !ptr )
		{
			return {};
		}
		return shared_ptr<T>( shared, ptr );
	}

	template <typename T, typename U>
	[[nodiscard]] shared_ptr<T> const_pointer_cast( const shared_ptr<U>& shared ) noexcept
	{
		auto ptr = const_cast<typename shared_ptr<T>::element_type*>( shared.get() );
		return shared_ptr<T>( shared, ptr );
	}

	template <typename T, typename U>
	[[nodiscard]] shared_ptr<T> reinterpret_pointer_cast( const shared_ptr<U>& shared ) noexcept
	{
		auto ptr = reinterpret_cast<typename shared_ptr<T>::element_type*>( shared.get() );
		return shared_ptr<T>( shared, ptr );
	}

	template <typename TDeleter, typename T>
	[[nodiscard]] TDeleter* get_deleter( const shared_ptr<T>& shared ) noexcept
	{
		if ( shared.control_block )
		{
			return static_cast<TDeleter*>( shared.control_block->get_deleter( typeid( TDeleter ) ) );
		}
		return nullptr;
	}

	template <typename CharT, typename Traits, typename T>
	std::basic_ostream<CharT, Traits>& operator<<( std::basic_ostream<CharT, Traits>& os, const shared_ptr<T>& ptr )
	{
		return os << ptr.get();
	}
}
