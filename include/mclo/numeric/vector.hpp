#pragma once

#include "mclo/container/span.hpp"
#include "mclo/functional/bind.hpp"
#include "mclo/utility/array.hpp"

#include <array>
#include <cmath>
#include <concepts>
#include <functional>

namespace mclo
{
	template <typename T, std::size_t N>
	class vec_base
	{
	public:
		using value_type = T;

		constexpr vec_base() noexcept = default;

		constexpr explicit vec_base( const T value ) noexcept
			: m_data{ broadcast_array<N>( value ) }
		{
		}

		template <typename... Ts>
			requires( sizeof...( Ts ) == N )
		constexpr explicit( ( !std::convertible_to<Ts, T> || ... ) ) vec_base( Ts... values ) noexcept
			: m_data{ static_cast<T>( values )... }
		{
		}

		constexpr explicit vec_base( const mclo::span<const T, N> values ) noexcept
			: m_data{ to_array( values ) }
		{
		}

		// Data access
		[[nodiscard]] constexpr T& operator[]( const std::size_t index ) noexcept
		{
			return m_data[ index ];
		}
		[[nodiscard]] constexpr T operator[]( const std::size_t index ) const noexcept
		{
			return m_data[ index ];
		}

		[[nodiscard]] constexpr T* data() noexcept
		{
			return m_data.data();
		}
		[[nodiscard]] constexpr const T* data() const noexcept
		{
			return m_data.data();
		}

		[[nodiscard]] static constexpr std::size_t size() noexcept
		{
			return N;
		}

		[[nodiscard]] constexpr operator mclo::span<T, N>() noexcept
		{
			return mclo::span( m_data );
		}
		[[nodiscard]] constexpr operator mclo::span<const T, N>() const noexcept
		{
			return mclo::span( m_data );
		}

		// Generic element-wise operations
		template <typename Func>
		[[nodiscard]] constexpr vec_base<std::invoke_result_t<Func, T>, N> map( Func func ) const noexcept
		{
			return map_internal( func, std::make_index_sequence<N>{} );
		}

		template <typename Func>
		[[nodiscard]] constexpr vec_base<std::invoke_result_t<Func, T, T>, N> map_with(
			Func func, const vec_base& other ) const noexcept
		{
			return map_with_internal( func, other, std::make_index_sequence<N>{} );
		}

		template <typename Func>
		[[nodiscard]] constexpr T fold_left( Func func, T initial = T( 0 ) ) const noexcept
		{
			return fold_left_internal( func, initial, std::make_index_sequence<N>{} );
		}

		template <typename Func>
		[[nodiscard]] constexpr T fold_right( Func func, T initial = T( 0 ) ) const noexcept
		{
			return fold_right_internal( func, initial, std::make_index_sequence<N>{} );
		}

		template <typename U>
		[[nodiscard]] constexpr vec_base<U, N> cast() const noexcept
		{
			return map( []( const T value ) { return static_cast<U>( value ); } );
		}

		// Element wise folds
		[[nodiscard]] constexpr T sum() const noexcept
		{
			return fold_left( std::plus{} );
		}

		[[nodiscard]] constexpr T product() const noexcept
		{
			return fold_left( std::multiplies{}, T( 1 ) );
		}

		[[nodiscard]] constexpr T mean() const noexcept
		{
			return sum() / N;
		}

		// Vector arithmetic operators
		[[nodiscard]] constexpr vec_base operator+( const vec_base& other ) const noexcept
		{
			return map_with( std::plus{}, other );
		}

		constexpr vec_base& operator+=( const vec_base& other ) noexcept
		{
			*this = *this + other;
			return static_cast<vec_base&>( *this );
		}

		[[nodiscard]] constexpr vec_base operator-( const vec_base& other ) const noexcept
		{
			return map_with( std::minus{}, other );
		}

		constexpr vec_base& operator-=( const vec_base& other ) noexcept
		{
			*this = *this - other;
			return static_cast<vec_base&>( *this );
		}

		[[nodiscard]] constexpr vec_base operator*( const vec_base& other ) const noexcept
		{
			return map_with( std::multiplies{}, other );
		}

		constexpr vec_base& operator*=( const vec_base& other ) noexcept
		{
			*this = *this * other;
			return static_cast<vec_base&>( *this );
		}

		[[nodiscard]] constexpr vec_base operator/( const vec_base& other ) const noexcept
		{
			return map_with( std::divides{}, other );
		}

		constexpr vec_base& operator/=( const vec_base& other ) noexcept
		{
			*this = *this / other;
			return static_cast<vec_base&>( *this );
		}

		[[nodiscard]] constexpr vec_base operator-() const noexcept
		{
			return map( std::negate{} );
		}

		// Scalr arithmetic operators
		[[nodiscard]] constexpr vec_base operator+( const T scalar ) const noexcept
		{
			return map( std::plus{}, scalar );
		}

		constexpr vec_base& operator+=( const T scalar ) noexcept
		{
			*this = *this + scalar;
			return static_cast<vec_base&>( *this );
		}

		[[nodiscard]] constexpr vec_base operator-( const T scalar ) const noexcept
		{
			return map( mclo::bind_back( std::minus{}, scalar ) );
		}

		constexpr vec_base& operator-=( const T scalar ) noexcept
		{
			*this = *this - scalar;
			return static_cast<vec_base&>( *this );
		}

		[[nodiscard]] constexpr vec_base operator*( const T scalar ) const noexcept
		{
			return map( mclo::bind_back( std::multiplies{}, scalar ) );
		}

		constexpr vec_base& operator*=( const T scalar ) noexcept
		{
			*this = *this * scalar;
			return static_cast<vec_base&>( *this );
		}

		[[nodiscard]] constexpr vec_base operator/( const T scalar ) const noexcept
		{
			return map( mclo::bind_back( std::divides{}, scalar ) );
		}

		constexpr vec_base& operator/=( const T scalar ) noexcept
		{
			*this = *this / scalar;
			return static_cast<vec_base&>( *this );
		}

		// Basic operations
		[[nodiscard]] constexpr vec_base abs() const noexcept
		{
			using std::abs;
			return map( []( const T value ) { return static_cast<T>( abs( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base reciprocal() const noexcept
		{
			return map( []( const T value ) { return T( 1 ) / value; } );
		}

		// Rounding operations
		[[nodiscard]] constexpr vec_base floor() const noexcept
		{
			using std::floor;
			return map( []( const T value ) { return static_cast<T>( floor( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base ceil() const noexcept
		{
			using std::ceil;
			return map( []( const T value ) { return static_cast<T>( ceil( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base round() const noexcept
		{
			using std::round;
			return map( []( const T value ) { return static_cast<T>( round( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base trunc() const noexcept
		{
			using std::trunc;
			return map( []( const T value ) { return static_cast<T>( trunc( value ) ); } );
		}

		// Exponent operations
		[[nodiscard]] constexpr vec_base exp() const noexcept
		{
			using std::exp;
			return map( []( const T value ) { return static_cast<T>( exp( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base log() const noexcept
		{
			using std::log;
			return map( []( const T value ) { return static_cast<T>( log( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base log2() const noexcept
		{
			using std::log2;
			return map( []( const T value ) { return static_cast<T>( log2( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base log10() const noexcept
		{
			using std::log10;
			return map( []( const T value ) { return static_cast<T>( log10( value ) ); } );
		}

		// Power operations
		[[nodiscard]] constexpr vec_base pow( const T exponent ) const noexcept
		{
			using std::pow;
			return map( [ exponent ]( const T value ) { return static_cast<T>( pow( value, exponent ) ); } );
		}

		[[nodiscard]] constexpr vec_base sqrt() const noexcept
		{
			using std::sqrt;
			return map( []( const T value ) { return static_cast<T>( sqrt( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base cbrt() const noexcept
		{
			using std::cbrt;
			return map( []( const T value ) { return static_cast<T>( cbrt( value ) ); } );
		}

		// Trigonmetric operations
		[[nodiscard]] constexpr vec_base sin() const noexcept
		{
			using std::sin;
			return map( []( const T value ) { return static_cast<T>( sin( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base cos() const noexcept
		{
			using std::cos;
			return map( []( const T value ) { return static_cast<T>( cos( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base tan() const noexcept
		{
			using std::tan;
			return map( []( const T value ) { return static_cast<T>( tan( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base asin() const noexcept
		{
			using std::asin;
			return map( []( const T value ) { return static_cast<T>( asin( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base acos() const noexcept
		{
			using std::acos;
			return map( []( const T value ) { return static_cast<T>( acos( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base atan() const noexcept
		{
			using std::atan;
			return map( []( const T value ) { return static_cast<T>( atan( value ) ); } );
		}

		// Hyperbolic functions

		[[nodiscard]] constexpr vec_base sinh() const noexcept
		{
			using std::sinh;
			return map( []( const T value ) { return static_cast<T>( sinh( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base cosh() const noexcept
		{
			using std::cosh;
			return map( []( const T value ) { return static_cast<T>( cosh( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base tanh() const noexcept
		{
			using std::tanh;
			return map( []( const T value ) { return static_cast<T>( tanh( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base asinh() const noexcept
		{
			using std::asinh;
			return map( []( const T value ) { return static_cast<T>( asinh( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base acosh() const noexcept
		{
			using std::acosh;
			return map( []( const T value ) { return static_cast<T>( acosh( value ) ); } );
		}

		[[nodiscard]] constexpr vec_base atanh() const noexcept
		{
			using std::atanh;
			return map( []( const T value ) { return static_cast<T>( atanh( value ) ); } );
		}

		// Geometry operations
		[[nodiscard]] constexpr T norm_squared() const noexcept
		{
			return map( []( const T value ) { return value * value; } ).fold_left( std::plus<>{} );
		}

		[[nodiscard]] constexpr T norm() const noexcept
		{
			using std::sqrt;
			return static_cast<T>( sqrt( norm_squared() ) );
		}

		[[nodiscard]] constexpr vec_base normalized() const noexcept
		{
			return *this / norm();
		}

		[[nodiscard]] constexpr T dot( const vec_base& other ) const noexcept
		{
			return ( *this * other ).fold_left( std::plus<>{} );
		}

		[[nodiscard]] constexpr T distance( const vec_base& other ) const noexcept
		{
			return ( *this - other ).norm();
		}

		[[nodiscard]] constexpr auto operator<=>( const vec_base& other ) const noexcept = default;

	private:
		template <typename Func, std::size_t... Indices>
		[[nodiscard]] constexpr vec_base<std::invoke_result_t<Func, T>, N> map_internal(
			Func func, std::index_sequence<Indices...> ) const noexcept
		{
			return { func( m_data[ Indices ] )... };
		}

		template <typename Func, std::size_t... Indices>
		[[nodiscard]] constexpr vec_base<std::invoke_result_t<Func, T, T>, N> map_with_internal(
			Func func, const vec_base& other, std::index_sequence<Indices...> ) const noexcept
		{
			return { func( m_data[ Indices ], other.m_data[ Indices ] )... };
		}

		template <typename Func, std::size_t... Indices>
		[[nodiscard]] constexpr T fold_left_internal( Func func,
													  T initial,
													  std::index_sequence<Indices...> ) const noexcept
		{
			( ..., ( initial = func( initial, m_data[ Indices ] ) ) );
			return initial;
		}

		template <typename Func, std::size_t... Indices>
		[[nodiscard]] constexpr T fold_right_internal( Func func,
													   T initial,
													   std::index_sequence<Indices...> ) const noexcept
		{
			( ( initial = func( initial, m_data[ Indices ] ) ), ... );
			return initial;
		}

		std::array<T, N> m_data{};
	};

#define MCLO_VEC_ACCESSOR( NAME, INDEX )                                                                               \
	[[nodiscard]] constexpr typename base::value_type& NAME() noexcept                                                 \
	{                                                                                                                  \
		return this->operator[]( INDEX );                                                                              \
	}                                                                                                                  \
	[[nodiscard]] constexpr typename base::value_type NAME() const noexcept                                            \
	{                                                                                                                  \
		return this->operator[]( INDEX );                                                                              \
	}

	template <typename T>
	class vec2 : public vec_base<T, 2>
	{
	private:
		using base = vec_base<T, 2>;

	public:
		using base::base;

		constexpr vec2( const base& value ) noexcept
			: base{ value }
		{
		}

		MCLO_VEC_ACCESSOR( x, 0 )
		MCLO_VEC_ACCESSOR( y, 1 )
	};

	template <typename T>
	class vec3 : public vec_base<T, 3>
	{
	private:
		using base = vec_base<T, 3>;

	public:
		using base::base;

		constexpr vec3( const base& value ) noexcept
			: base{ value }
		{
		}

		MCLO_VEC_ACCESSOR( x, 0 )
		MCLO_VEC_ACCESSOR( y, 1 )
		MCLO_VEC_ACCESSOR( z, 2 )

		[[nodiscard]] constexpr vec3 cross( const vec3& other ) const noexcept
		{
			return { y() * other.z() - z() * other.y(),
					 z() * other.x() - x() * other.z(),
					 x() * other.y() - y() * other.x() };
		}

		[[nodiscard]] constexpr T angle( const vec3& other ) const noexcept
		{
			using std::acos;
			return acos( this->dot( other ) / ( this->norm() * other.norm() ) );
		}
	};

	template <typename T>
	class vec4 : public vec_base<T, 4>
	{
	private:
		using base = vec_base<T, 4>;

	public:
		using base::base;

		constexpr vec4( const base& value ) noexcept
			: base{ value }
		{
		}

		MCLO_VEC_ACCESSOR( x, 0 )
		MCLO_VEC_ACCESSOR( y, 1 )
		MCLO_VEC_ACCESSOR( z, 2 )
		MCLO_VEC_ACCESSOR( w, 3 )
	};

	template <typename T>
	concept vec_type = std::derived_from<T, mclo::vec_base<typename T::value_type, T::size()>>;

	using vec2i = vec2<int>;
	using vec3i = vec3<int>;
	using vec4i = vec4<int>;

	using vec2f = vec2<float>;
	using vec3f = vec3<float>;
	using vec4f = vec4<float>;

	using vec2d = vec2<double>;
	using vec3d = vec3<double>;
	using vec4d = vec4<double>;
}
