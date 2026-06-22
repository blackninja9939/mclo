#pragma once

#include <memory>

namespace mclo
{
	/// @brief A proxy that returns a temporary by value from @c operator-> for proxy iterators.
	/// @details Iterators whose @c operator* yields a value (a proxy reference) rather than a true reference cannot
	/// return a raw pointer from @c operator->. Wrapping the value in an @c arrow_proxy keeps it alive for the
	/// duration of the member-access expression and exposes its address, satisfying the iterator @c operator->
	/// requirements.
	/// @tparam Reference The (possibly proxy) reference type to hold by value.
	template <typename Reference>
	struct arrow_proxy
	{
		/// @brief The held value that member access is forwarded to.
		Reference r;

		/// @brief Returns the address of the held value @ref r.
		/// @return A pointer to @ref r.
		Reference* operator->()
		{
			return std::addressof( r );
		}
	};
}
