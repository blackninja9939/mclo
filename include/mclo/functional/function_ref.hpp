#pragma once

#include "mclo/functional/nontype.hpp"

#include "mclo/debug/assert.hpp"

#include <functional>

#if defined( __cpp_lib_function_ref ) && ( __cpp_lib_function_ref >= 202306L )

namespace mclo
{
	/// @brief A non-owning wrapper referring to any callable target with a matching call signature.
	using std::function_ref;
}

#else

#include <type_traits>
#include <utility>

namespace mclo
{
	template <typename... Signature>
	class function_ref;
}

namespace mclo::detail
{
	// Trivially copyable erased storage able to hold either a pointer to object or a pointer to function.
	union function_ref_storage
	{
		void* obj = nullptr;
		void ( *fn )();
	};

	// Maps the callable baked into a nontype_t (with a bound object of type Obj) to the call signature of the
	// resulting function_ref. Pointers to functions and member functions have the implicit object parameter stripped
	// and any noexcept-specifier preserved; a pointer to member data yields R() where R is the type obtained by
	// accessing the member through the bound object.
	template <typename Callable, typename Obj>
	struct function_ref_bound_signature;

	template <typename R, typename First, typename... Rest, typename Obj>
	struct function_ref_bound_signature<R ( * )( First, Rest... ), Obj>
	{
		using type = R( Rest... );
	};

	template <typename R, typename First, typename... Rest, typename Obj>
	struct function_ref_bound_signature<R ( * )( First, Rest... ) noexcept, Obj>
	{
		using type = R( Rest... ) noexcept;
	};

	template <typename M, typename C, typename Obj>
		requires std::is_object_v<M>
	struct function_ref_bound_signature<M C::*, Obj>
	{
		// The trailing () forms the nullary function type R(), where R is the result of accessing the member through
		// the bound object.
		using type = std::invoke_result_t<M C::*, Obj&>();
	};

#define MCLO_DETAIL_FUNCTION_REF_BOUND_MEMBER( CV, REF, NOEXCEPT_KW )                                                  \
	template <typename R, typename C, typename... G, typename Obj>                                                     \
	struct function_ref_bound_signature<R ( C::* )( G... ) CV REF NOEXCEPT_KW, Obj>                                    \
	{                                                                                                                  \
		using type = R( G... ) NOEXCEPT_KW;                                                                            \
	};

	MCLO_DETAIL_FUNCTION_REF_BOUND_MEMBER(, , )
	MCLO_DETAIL_FUNCTION_REF_BOUND_MEMBER(, &, )
	MCLO_DETAIL_FUNCTION_REF_BOUND_MEMBER( const, , )
	MCLO_DETAIL_FUNCTION_REF_BOUND_MEMBER( const, &, )
	MCLO_DETAIL_FUNCTION_REF_BOUND_MEMBER(, , noexcept )
	MCLO_DETAIL_FUNCTION_REF_BOUND_MEMBER(, &, noexcept )
	MCLO_DETAIL_FUNCTION_REF_BOUND_MEMBER( const, , noexcept )
	MCLO_DETAIL_FUNCTION_REF_BOUND_MEMBER( const, &, noexcept )

#undef MCLO_DETAIL_FUNCTION_REF_BOUND_MEMBER

	// Shared implementation of every function_ref specialization. function_ref's call operator is always const, so the
	// const-qualifier and noexcept-specifier of the signature only influence how the target is invoked and whether the
	// stored thunk is noexcept. These are threaded through as the IsConst and IsNoexcept flags rather than requiring a
	// separate class body per combination.
	template <typename R, bool IsConst, bool IsNoexcept, typename... Args>
	class function_ref_impl
	{
		using storage = function_ref_storage;
		using thunk_ptr =
			std::conditional_t<IsNoexcept, R ( * )( storage, Args&&... ) noexcept, R ( * )( storage, Args&&... )>;

		template <typename... CallArgs>
		static constexpr bool is_invocable_using = IsNoexcept ? std::is_nothrow_invocable_r_v<R, CallArgs..., Args...>
															  : std::is_invocable_r_v<R, CallArgs..., Args...>;

		template <typename T>
		using target_ptr = std::conditional_t<IsConst, const T*, T*>;

		template <typename T>
		using target_ref = std::conditional_t<IsConst, const T&, T&>;

	public:
		template <typename F>
			requires( std::is_function_v<F> && is_invocable_using<F> )
		function_ref_impl( F* f ) noexcept
		{
			MCLO_DEBUG_ASSERT( f != nullptr, "function_ref constructed from a null function pointer" );
			m_bound.fn = reinterpret_cast<void ( * )()>( f );
			m_thunk = []( storage bound, Args&&... args ) noexcept( IsNoexcept ) -> R {
				const auto target = reinterpret_cast<F*>( bound.fn );
				if constexpr ( std::is_void_v<R> )
				{
					std::invoke( target, std::forward<Args>( args )... );
				}
				else
				{
					return std::invoke( target, std::forward<Args>( args )... );
				}
			};
		}

		template <typename F>
			requires(
				!std::is_same_v<std::remove_cvref_t<F>, function_ref_impl>
				&& !detail::nontype_specialization<std::remove_cvref_t<F>>
				&& !std::is_member_pointer_v<std::remove_reference_t<F>>
				&& is_invocable_using<target_ref<std::remove_reference_t<F>>>
			)
		function_ref_impl( F&& f ) noexcept
		{
			using T = std::remove_reference_t<F>;
			m_bound.obj = const_cast<void*>( static_cast<const void*>( std::addressof( f ) ) );
			m_thunk = []( storage bound, Args&&... args ) noexcept( IsNoexcept ) -> R {
				const auto target = static_cast<target_ptr<T>>( bound.obj );
				if constexpr ( std::is_void_v<R> )
				{
					std::invoke( *target, std::forward<Args>( args )... );
				}
				else
				{
					return std::invoke( *target, std::forward<Args>( args )... );
				}
			};
		}

		template <auto f>
			requires( is_invocable_using<decltype( f )> )
		function_ref_impl( nontype_t<f> ) noexcept
		{
			using F = decltype( f );
			if constexpr ( std::is_pointer_v<F> || std::is_member_pointer_v<F> )
			{
				MCLO_DEBUG_ASSERT( f != nullptr, "function_ref constructed from a null pointer" );
			}
			m_bound.obj = nullptr;
			m_thunk = []( storage, Args&&... args ) noexcept( IsNoexcept ) -> R {
				if constexpr ( std::is_void_v<R> )
				{
					std::invoke( f, std::forward<Args>( args )... );
				}
				else
				{
					return std::invoke( f, std::forward<Args>( args )... );
				}
			};
		}

		template <auto f, typename U>
			requires(
				!std::is_rvalue_reference_v<U &&>
				&& is_invocable_using<decltype( f ), target_ref<std::remove_reference_t<U>>>
			)
		function_ref_impl( nontype_t<f>, U&& obj ) noexcept
		{
			using F = decltype( f );
			using T = std::remove_reference_t<U>;
			if constexpr ( std::is_pointer_v<F> || std::is_member_pointer_v<F> )
			{
				MCLO_DEBUG_ASSERT( f != nullptr, "function_ref constructed from a null pointer" );
			}
			m_bound.obj = const_cast<void*>( static_cast<const void*>( std::addressof( obj ) ) );
			m_thunk = []( storage bound, Args&&... args ) noexcept( IsNoexcept ) -> R {
				const auto target = static_cast<target_ptr<T>>( bound.obj );
				if constexpr ( std::is_void_v<R> )
				{
					std::invoke( f, *target, std::forward<Args>( args )... );
				}
				else
				{
					return std::invoke( f, *target, std::forward<Args>( args )... );
				}
			};
		}

		template <auto f, typename T>
			requires( is_invocable_using<decltype( f ), target_ptr<T>> )
		function_ref_impl( nontype_t<f>, T* obj ) noexcept
		{
			using F = decltype( f );
			if constexpr ( std::is_pointer_v<F> || std::is_member_pointer_v<F> )
			{
				MCLO_DEBUG_ASSERT( f != nullptr, "function_ref constructed from a null pointer" );
			}
			m_bound.obj = const_cast<void*>( static_cast<const void*>( obj ) );
			m_thunk = []( storage bound, Args&&... args ) noexcept( IsNoexcept ) -> R {
				const auto target = static_cast<target_ptr<T>>( bound.obj );
				if constexpr ( std::is_void_v<R> )
				{
					std::invoke( f, target, std::forward<Args>( args )... );
				}
				else
				{
					return std::invoke( f, target, std::forward<Args>( args )... );
				}
			};
		}

		constexpr function_ref_impl( const function_ref_impl& ) noexcept = default;
		constexpr function_ref_impl& operator=( const function_ref_impl& ) noexcept = default;

		template <typename T>
			requires( !std::is_same_v<std::remove_cvref_t<T>, function_ref_impl> && !std::is_pointer_v<T>
					  && !detail::nontype_specialization<T> )
		function_ref_impl& operator=( T ) = delete;

		R operator()( Args... args ) const noexcept( IsNoexcept )
		{
			return m_thunk( m_bound, std::forward<Args>( args )... );
		}

	private:
		storage m_bound{};
		thunk_ptr m_thunk = nullptr;
	};
}

namespace mclo
{
	/// @brief A non-owning wrapper referring to any callable target with a matching call signature.
	/// @details A polyfill for C++26 @c std::function_ref, used when the standard library does not provide it. It
	/// stores a reference to a callable together with a thunk that invokes it, without owning or copying the target.
	/// The const-qualifier and noexcept-specifier of the call signature are honoured; ref-qualified and C-variadic
	/// signatures are not supported. See @c std::function_ref for the full interface.
	/// @tparam R The return type of the call signature.
	/// @tparam Args The parameter types of the call signature.
	template <typename R, typename... Args>
	class function_ref<R( Args... )> : public detail::function_ref_impl<R, false, false, Args...>
	{
		using base = detail::function_ref_impl<R, false, false, Args...>;

	public:
		using base::base;
		using base::operator=;
	};

	template <typename R, typename... Args>
	class function_ref<R( Args... ) noexcept> : public detail::function_ref_impl<R, false, true, Args...>
	{
		using base = detail::function_ref_impl<R, false, true, Args...>;

	public:
		using base::base;
		using base::operator=;
	};

	template <typename R, typename... Args>
	class function_ref<R( Args... ) const> : public detail::function_ref_impl<R, true, false, Args...>
	{
		using base = detail::function_ref_impl<R, true, false, Args...>;

	public:
		using base::base;
		using base::operator=;
	};

	template <typename R, typename... Args>
	class function_ref<R( Args... ) const noexcept> : public detail::function_ref_impl<R, true, true, Args...>
	{
		using base = detail::function_ref_impl<R, true, true, Args...>;

	public:
		using base::base;
		using base::operator=;
	};

	template <typename F>
		requires std::is_function_v<F>
	function_ref( F* ) -> function_ref<F>;

	template <auto f>
		requires std::is_function_v<std::remove_pointer_t<decltype( f )>>
	function_ref( nontype_t<f> ) -> function_ref<std::remove_pointer_t<decltype( f )>>;

	template <auto f, typename T>
	function_ref( nontype_t<f>, T&& )
		-> function_ref<typename detail::function_ref_bound_signature<decltype( f ), T>::type>;
}

#endif
