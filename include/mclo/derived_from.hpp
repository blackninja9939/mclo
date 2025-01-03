#pragma once

namespace mclo
{
	namespace detail
	{
		template <template <typename...> class Template, typename... Args>
		void derived_from_specialization( const Template<Args...>& );
	}

	template <typename T, template <typename...> class Template>
	concept derived_from_specialization =
		requires( const T& object ) { detail::derived_from_specialization<Template>( object ); };

}
