#pragma once

#ifdef __cpp_static_call_operator
#define MCLO_STATIC_CALL_OPERATOR static
#define MCLO_CONST_CALL_OPERATOR
#else
#define MCLO_STATIC_CALL_OPERATOR
#define MCLO_CONST_CALL_OPERATOR const
#endif
