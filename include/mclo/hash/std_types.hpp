#pragma once

#include "mclo/hash/hash_append.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <typeindex>
#include <variant>

namespace mclo
{
	template <mclo::hasher Hasher, typename CharT, typename Traits, typename Alloc>
	void hash_append( Hasher& hasher, const std::basic_string<CharT, Traits, Alloc>& value ) noexcept
	{
		hasher.write( std::as_bytes( std::span( value.data(), value.size() ) ) );
	}

	template <mclo::hasher Hasher, typename CharT, typename Traits>
	void hash_append( Hasher& hasher, const std::basic_string_view<CharT, Traits> value ) noexcept
	{
		hasher.write( std::as_bytes( std::span( value.data(), value.size() ) ) );
	}

	template <mclo::hasher Hasher, mclo::hashable_with<Hasher> T>
	void hash_append( Hasher& hasher, const std::optional<T> value ) noexcept
	{
		if ( value )
		{
			hash_append( hasher, *value );
		}
	}

	template <mclo::hasher Hasher, typename T, typename Deleter>
	void hash_append( Hasher& hasher, const std::unique_ptr<T, Deleter>& value ) noexcept
	{
		hash_append( hasher, value.get() );
	}

	template <mclo::hasher Hasher, typename T>
	void hash_append( Hasher& hasher, const std::shared_ptr<T>& value ) noexcept
	{
		hash_append( hasher, value.get() );
	}

	template <mclo::hasher Hasher, mclo::hashable_with<Hasher>... Ts>
	void hash_append( Hasher& hasher, const std::variant<Ts...>& value ) noexcept
	{
		hash_append( hasher, value.index() );
		std::visit( [ & ]( const auto& v ) { hash_append( hasher, v ); }, value );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::monostate& value ) noexcept
	{
		hash_append( hasher, 1883 ); // Random number
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::type_index& value ) noexcept
	{
		hash_append( hasher, value.hash_code() );
	}
}
