#include "consteval_check.h"

#include "mclo/math.hpp"

TEST_CASE( "ceil_divide", "[math]" )
{
	CONSTEVAL_CHECK( mclo::ceil_divide( 6, 4 ) == 2 );
	CONSTEVAL_CHECK( mclo::ceil_divide( 6, -4 ) == -1 );
	CONSTEVAL_CHECK( mclo::ceil_divide( -6, 4 ) == -1 );
	CONSTEVAL_CHECK( mclo::ceil_divide( -6, -4 ) == 2 );
}
