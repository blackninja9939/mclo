#include "consteval_check.hpp"

#include "mclo/numeric/triangular_layout.hpp"

TEST_CASE( "triangular_size, known values", "[math][triangular_layout]" )
{
	CONSTEVAL_CHECK( mclo::triangular_size( 0 ) == 0 );
	CONSTEVAL_CHECK( mclo::triangular_size( 1 ) == 1 );
	CONSTEVAL_CHECK( mclo::triangular_size( 2 ) == 3 );
	CONSTEVAL_CHECK( mclo::triangular_size( 3 ) == 6 );
	CONSTEVAL_CHECK( mclo::triangular_size( 4 ) == 10 );
	CONSTEVAL_CHECK( mclo::triangular_size( 5 ) == 15 );
	CONSTEVAL_CHECK( mclo::triangular_size( 10 ) == 55 );
}

TEST_CASE( "triangular_index, lower triangle ordering", "[math][triangular_layout]" )
{
	// Row 0
	CONSTEVAL_CHECK( mclo::triangular_index( 0, 0 ) == 0 );

	// Row 1
	CONSTEVAL_CHECK( mclo::triangular_index( 1, 0 ) == 1 );
	CONSTEVAL_CHECK( mclo::triangular_index( 1, 1 ) == 2 );

	// Row 2
	CONSTEVAL_CHECK( mclo::triangular_index( 2, 0 ) == 3 );
	CONSTEVAL_CHECK( mclo::triangular_index( 2, 1 ) == 4 );
	CONSTEVAL_CHECK( mclo::triangular_index( 2, 2 ) == 5 );

	// Row 3
	CONSTEVAL_CHECK( mclo::triangular_index( 3, 0 ) == 6 );
	CONSTEVAL_CHECK( mclo::triangular_index( 3, 2 ) == 8 );
}

TEST_CASE( "triangular_index, symmetric", "[math][triangular_layout]" )
{
	CONSTEVAL_CHECK( mclo::triangular_index( 3, 1 ) == mclo::triangular_index( 1, 3 ) );
	CONSTEVAL_CHECK( mclo::triangular_index( 4, 2 ) == mclo::triangular_index( 2, 4 ) );
	CONSTEVAL_CHECK( mclo::triangular_index( 0, 5 ) == mclo::triangular_index( 5, 0 ) );
}

TEST_CASE( "triangular_row_column, known values", "[math][triangular_layout]" )
{
	CHECK( mclo::triangular_row_column( 0 ) == std::pair{ std::size_t( 0 ), std::size_t( 0 ) } );
	CHECK( mclo::triangular_row_column( 1 ) == std::pair{ std::size_t( 1 ), std::size_t( 0 ) } );
	CHECK( mclo::triangular_row_column( 2 ) == std::pair{ std::size_t( 1 ), std::size_t( 1 ) } );
	CHECK( mclo::triangular_row_column( 3 ) == std::pair{ std::size_t( 2 ), std::size_t( 0 ) } );
	CHECK( mclo::triangular_row_column( 5 ) == std::pair{ std::size_t( 2 ), std::size_t( 2 ) } );
	CHECK( mclo::triangular_row_column( 9 ) == std::pair{ std::size_t( 3 ), std::size_t( 3 ) } );
}

TEST_CASE( "triangular_row_column, roundtrip with triangular_index", "[math][triangular_layout]" )
{
	for ( std::size_t row = 0; row < 20; ++row )
	{
		for ( std::size_t column = 0; column <= row; ++column )
		{
			const std::size_t index = mclo::triangular_index( row, column );
			const auto [ r, c ] = mclo::triangular_row_column( index );
			CHECK( r == row );
			CHECK( c == column );
		}
	}
}
