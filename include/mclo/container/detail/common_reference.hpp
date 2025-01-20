#pragma once

#include <type_traits>
#include <utility>

#if __cplusplus < 202302L

template <class T1, class T2, class U1, class U2, template <class> class TQual, template <class> class UQual>
	requires requires {
		typename std::pair<std::common_reference_t<TQual<T1>, UQual<U1>>,
						   std::common_reference_t<TQual<T2>, UQual<U2>>>;
	}

struct std::basic_common_reference<std::pair<T1, T2>, std::pair<U1, U2>, TQual, UQual>
{
	using type =
		std::pair<std::common_reference_t<TQual<T1>, UQual<U1>>, std::common_reference_t<TQual<T2>, UQual<U2>>>;
};

#endif
