#pragma once

#include "mclo/concepts/specialization_of.hpp"
#include "mclo/debug/assert.hpp"

#include <functional>

#if defined( __cpp_lib_move_only_function ) && ( __cpp_lib_move_only_function >= 202110L )

namespace mclo
{
	/// @brief A move-only owning wrapper of any callable target with a matching, qualifier-aware call signature.
	using std::move_only_function;
}

#else

#include <cstddef>
#include <initializer_list>
#include <new>
#include <type_traits>
#include <utility>

namespace mclo
{
	template <typename... Signature>
	class move_only_function;
}

namespace mclo::detail
{
	// The number of bytes available for storing a target inline before falling back to heap allocation.
	inline constexpr std::size_t move_only_function_buffer_size = 3 * sizeof( void* );

	// Type-erased storage for a move_only_function target: either a heap pointer or an inline aligned buffer.
	union move_only_function_storage
	{
		void* heap;
		alignas( std::max_align_t ) std::byte buffer[ move_only_function_buffer_size ];
	};

	// A target is stored inline only when it fits the buffer and its move constructor cannot throw, so that moving a
	// move_only_function stays noexcept.
	template <typename T>
	inline constexpr bool move_only_function_stored_inline =
		sizeof( T ) <= move_only_function_buffer_size && alignof( T ) <= alignof( move_only_function_storage ) &&
		std::is_nothrow_move_constructible_v<T>;

	template <typename T>
	[[nodiscard]] T* move_only_function_target( move_only_function_storage& storage ) noexcept
	{
		if constexpr ( move_only_function_stored_inline<T> )
		{
			return std::launder( reinterpret_cast<T*>( storage.buffer ) );
		}
		else
		{
			return static_cast<T*>( storage.heap );
		}
	}

	// Non-signature-dependent operations for relocating and destroying a target, shared across every specialization.
	struct move_only_function_vtable
	{
		void ( *move )( move_only_function_storage& dst, move_only_function_storage& src ) noexcept;
		void ( *destroy )( move_only_function_storage& self ) noexcept;
	};

	template <typename T>
	void move_only_function_do_move( move_only_function_storage& dst, move_only_function_storage& src ) noexcept
	{
		if constexpr ( move_only_function_stored_inline<T> )
		{
			T* const from = move_only_function_target<T>( src );
			::new ( static_cast<void*>( dst.buffer ) ) T( std::move( *from ) );
			from->~T();
		}
		else
		{
			dst.heap = src.heap;
		}
	}

	template <typename T>
	void move_only_function_do_destroy( move_only_function_storage& self ) noexcept
	{
		if constexpr ( move_only_function_stored_inline<T> )
		{
			move_only_function_target<T>( self )->~T();
		}
		else
		{
			delete move_only_function_target<T>( self );
		}
	}

	template <typename T>
	inline constexpr move_only_function_vtable move_only_function_vtable_for = { &move_only_function_do_move<T>,
																				 &move_only_function_do_destroy<T> };

	// Owns the storage, target lifetime, and move-only semantics; shared by every move_only_function_impl regardless of
	// signature qualifiers.
	class move_only_function_base
	{
	public:
		move_only_function_base() = default;

		move_only_function_base( move_only_function_base&& other ) noexcept
			: m_vtable( other.m_vtable )
			, m_call( other.m_call )
		{
			if ( m_vtable )
			{
				m_vtable->move( m_storage, other.m_storage );
			}
			other.m_vtable = nullptr;
			other.m_call = nullptr;
		}

		move_only_function_base& operator=( move_only_function_base&& other ) noexcept
		{
			if ( this != &other )
			{
				reset();
				m_vtable = other.m_vtable;
				m_call = other.m_call;
				if ( m_vtable )
				{
					m_vtable->move( m_storage, other.m_storage );
				}
				other.m_vtable = nullptr;
				other.m_call = nullptr;
			}
			return *this;
		}

		move_only_function_base( const move_only_function_base& ) = delete;
		move_only_function_base& operator=( const move_only_function_base& ) = delete;

		~move_only_function_base()
		{
			reset();
		}

	protected:
		using generic_fn = void ( * )();

		template <typename T, typename... CtorArgs>
		void construct_target( const generic_fn call, CtorArgs&&... args )
		{
			if constexpr ( move_only_function_stored_inline<T> )
			{
				::new ( static_cast<void*>( m_storage.buffer ) ) T( std::forward<CtorArgs>( args )... );
			}
			else
			{
				m_storage.heap = new T( std::forward<CtorArgs>( args )... );
			}
			m_vtable = &move_only_function_vtable_for<T>;
			m_call = call;
		}

		void reset() noexcept
		{
			if ( m_vtable )
			{
				m_vtable->destroy( m_storage );
				m_vtable = nullptr;
				m_call = nullptr;
			}
		}

		void swap_impl( move_only_function_base& other ) noexcept
		{
			move_only_function_base tmp( std::move( other ) );
			other = std::move( *this );
			*this = std::move( tmp );
		}

		[[nodiscard]] bool has_target() const noexcept
		{
			return m_vtable != nullptr;
		}

		move_only_function_storage m_storage;
		const move_only_function_vtable* m_vtable = nullptr;
		generic_fn m_call = nullptr;
	};

	// Shared implementation of every move_only_function specialization. The cv-qualifier, ref-qualifier and
	// noexcept-specifier of the signature only affect the invocation qualifiers of the target (the inv-quals) and
	// whether invocation is noexcept, so they are threaded through as flags rather than requiring a separate class body
	// per combination. Only the qualifiers of the call operator itself must be written per specialization, since C++20
	// lacks deducing-this; the derived specialization supplies that single member and forwards here.
	template <typename R, bool IsConst, bool IsRvalue, bool IsNoexcept, typename... Args>
	class move_only_function_impl : public move_only_function_base
	{
		using base = move_only_function_base;
		using invoke_ptr = std::conditional_t<IsNoexcept,
											  R ( * )( move_only_function_storage&, Args&&... ) noexcept,
											  R ( * )( move_only_function_storage&, Args&&... )>;

		// The inv-quals reference type: const if the signature is const-qualified, and an rvalue reference if the
		// signature is &&-qualified, otherwise an lvalue reference.
		template <typename VT>
		using inv_quals_t = std::conditional_t<IsRvalue,
											   std::conditional_t<IsConst, const VT&&, VT&&>,
											   std::conditional_t<IsConst, const VT&, VT&>>;

		template <typename VT>
		static constexpr bool is_callable_from = IsNoexcept ? std::is_nothrow_invocable_r_v<R, inv_quals_t<VT>, Args...>
															: std::is_invocable_r_v<R, inv_quals_t<VT>, Args...>;

		template <typename VT>
		static R do_invoke( move_only_function_storage& storage, Args&&... args ) noexcept( IsNoexcept )
		{
			VT* const target = move_only_function_target<VT>( storage );
			if constexpr ( std::is_void_v<R> )
			{
				std::invoke( static_cast<inv_quals_t<VT>>( *target ), std::forward<Args>( args )... );
			}
			else
			{
				return std::invoke( static_cast<inv_quals_t<VT>>( *target ), std::forward<Args>( args )... );
			}
		}

	protected:
		// Reaches the erased target and invokes it. The signature's cv/ref-qualifiers are compile-time overload
		// controls on the derived call operator; at runtime the invocation is identical, so a single const helper
		// serves every qualifier combination.
		R invoke_target( Args&&... args ) const noexcept( IsNoexcept )
		{
			MCLO_DEBUG_ASSERT( has_target(), "Invoking an empty move_only_function" );
			const auto call = reinterpret_cast<invoke_ptr>( m_call );
			return call( const_cast<move_only_function_storage&>( m_storage ), std::forward<Args>( args )... );
		}

	public:
		using result_type = R;

		move_only_function_impl() noexcept
		{
		}
		move_only_function_impl( std::nullptr_t ) noexcept
		{
		}

		move_only_function_impl( move_only_function_impl&& ) noexcept = default;
		move_only_function_impl& operator=( move_only_function_impl&& ) noexcept = default;

		template <typename F>
			requires( !std::is_same_v<std::remove_cvref_t<F>, move_only_function_impl> &&
					  !mclo::specialization_of<std::remove_cvref_t<F>, std::in_place_type_t> &&
					  is_callable_from<std::decay_t<F>> )
		move_only_function_impl( F&& f )
		{
			using VT = std::decay_t<F>;
			if constexpr ( std::is_pointer_v<VT> || std::is_member_pointer_v<VT> ||
						   mclo::specialization_of<VT, mclo::move_only_function> )
			{
				if ( f == nullptr )
				{
					return;
				}
			}
			construct_target<VT>( reinterpret_cast<generic_fn>( &do_invoke<VT> ), std::forward<F>( f ) );
		}

		template <typename T, typename... CtorArgs>
			requires( std::is_constructible_v<std::decay_t<T>, CtorArgs...> && is_callable_from<std::decay_t<T>> &&
					  std::is_same_v<std::decay_t<T>, T> )
		explicit move_only_function_impl( std::in_place_type_t<T>, CtorArgs&&... args )
		{
			using VT = std::decay_t<T>;
			construct_target<VT>( reinterpret_cast<generic_fn>( &do_invoke<VT> ), std::forward<CtorArgs>( args )... );
		}

		template <typename T, typename U, typename... CtorArgs>
			requires( std::is_constructible_v<std::decay_t<T>, std::initializer_list<U>&, CtorArgs...> &&
					  is_callable_from<std::decay_t<T>> && std::is_same_v<std::decay_t<T>, T> )
		explicit move_only_function_impl( std::in_place_type_t<T>, std::initializer_list<U> il, CtorArgs&&... args )
		{
			using VT = std::decay_t<T>;
			construct_target<VT>(
				reinterpret_cast<generic_fn>( &do_invoke<VT> ), il, std::forward<CtorArgs>( args )... );
		}

		move_only_function_impl& operator=( std::nullptr_t ) noexcept
		{
			reset();
			return *this;
		}

		template <typename F>
			requires std::is_constructible_v<move_only_function_impl, F>
		move_only_function_impl& operator=( F&& f )
		{
			move_only_function_impl( std::forward<F>( f ) ).swap_impl( *this );
			return *this;
		}

		void swap( move_only_function_impl& other ) noexcept
		{
			swap_impl( other );
		}

		friend void swap( move_only_function_impl& lhs, move_only_function_impl& rhs ) noexcept
		{
			lhs.swap_impl( rhs );
		}

		[[nodiscard]] explicit operator bool() const noexcept
		{
			return has_target();
		}

		[[nodiscard]] friend bool operator==( const move_only_function_impl& f, std::nullptr_t ) noexcept
		{
			return !f;
		}
	};
}

// Stamps out the thin move_only_function specialization for a given qualifier combination. Only the single qualified
// call operator differs between specializations; everything else is inherited from move_only_function_impl. CV is
// 'const' or empty, REF is '&', '&&' or empty, and NOEXCEPT_KW is 'noexcept' or empty. IS_CONST, IS_RVALUE and
// IS_NOEXCEPT are the matching bools configuring the shared base.
#define MCLO_DETAIL_MOVE_ONLY_FUNCTION( CV, REF, NOEXCEPT_KW, IS_CONST, IS_RVALUE, IS_NOEXCEPT )                       \
	template <typename R, typename... Args>                                                                            \
	class move_only_function<R( Args... ) CV REF NOEXCEPT_KW>                                                          \
		: public detail::move_only_function_impl<R, IS_CONST, IS_RVALUE, IS_NOEXCEPT, Args...>                         \
	{                                                                                                                  \
		using base = detail::move_only_function_impl<R, IS_CONST, IS_RVALUE, IS_NOEXCEPT, Args...>;                    \
                                                                                                                       \
	public:                                                                                                            \
		using base::base;                                                                                              \
		using base::operator=;                                                                                         \
                                                                                                                       \
		R operator()( Args... args ) CV REF NOEXCEPT_KW                                                                \
		{                                                                                                              \
			return this->invoke_target( std::forward<Args>( args )... );                                               \
		}                                                                                                              \
	};

namespace mclo
{
	/// @brief A move-only owning wrapper of any callable target with a matching, qualifier-aware call signature.
	/// @details A polyfill for C++26 @c std::move_only_function, used when the standard library does not provide it. It
	/// owns its target, storing small targets inline and heap-allocating larger ones, and supports every combination of
	/// cv-qualifier, ref-qualifier and noexcept-specifier on the call signature (C-variadic signatures are not
	/// supported). See @c std::move_only_function for the full interface.
	/// @tparam R The return type of the call signature.
	/// @tparam Args The parameter types of the call signature.
	MCLO_DETAIL_MOVE_ONLY_FUNCTION(, , , false, false, false )
	MCLO_DETAIL_MOVE_ONLY_FUNCTION(, , noexcept, false, false, true )
	MCLO_DETAIL_MOVE_ONLY_FUNCTION(, &, , false, false, false )
	MCLO_DETAIL_MOVE_ONLY_FUNCTION(, &, noexcept, false, false, true )
	MCLO_DETAIL_MOVE_ONLY_FUNCTION(, &&, , false, true, false )
	MCLO_DETAIL_MOVE_ONLY_FUNCTION(, &&, noexcept, false, true, true )
	MCLO_DETAIL_MOVE_ONLY_FUNCTION( const, , , true, false, false )
	MCLO_DETAIL_MOVE_ONLY_FUNCTION( const, , noexcept, true, false, true )
	MCLO_DETAIL_MOVE_ONLY_FUNCTION( const, &, , true, false, false )
	MCLO_DETAIL_MOVE_ONLY_FUNCTION( const, &, noexcept, true, false, true )
	MCLO_DETAIL_MOVE_ONLY_FUNCTION( const, &&, , true, true, false )
	MCLO_DETAIL_MOVE_ONLY_FUNCTION( const, &&, noexcept, true, true, true )
}

#undef MCLO_DETAIL_MOVE_ONLY_FUNCTION

#endif
