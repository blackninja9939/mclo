#include <benchmark/benchmark.h>

#include "mclo/string/compare_ignore_case.hpp"
#include "mclo/string/concatenate.hpp"

#include <random>
#include <string>

namespace
{
	constexpr std::string_view lorem_ipsum =
		R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse et diam vel lectus sodales bibendum. Ut rhoncus mi tellus, in feugiat mauris dictum sit amet. In accumsan feugiat quam id suscipit. Praesent vel volutpat justo. Quisque eu magna eu urna ultrices finibus. Phasellus vehicula consequat vehicula. Nunc at blandit ante. Sed hendrerit, ante at sagittis rhoncus, arcu neque luctus elit, a malesuada erat justo non nisl. In rutrum tincidunt eleifend. In malesuada eros nibh, non pellentesque erat pharetra ac. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Integer sodales magna et malesuada blandit. Sed sodales erat quis nibh aliquam, eget vehicula nulla mattis.

Interdum et malesuada fames ac ante ipsum primis in faucibus. Quisque sit amet nisl et est fermentum lacinia. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Praesent volutpat facilisis vehicula. Nunc id nulla accumsan, tristique odio quis, iaculis erat. Nullam vehicula maximus odio, ut pellentesque orci eleifend in. Etiam sit amet placerat dolor, vel blandit enim. Ut ac nisl tellus. Aenean nisl odio, ornare nec felis at, imperdiet pharetra nulla. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Nunc pellentesque interdum est, ac fringilla est auctor quis.

Proin consequat magna ac vehicula volutpat. Mauris tincidunt magna eget porta faucibus. Nulla sed pellentesque eros. Nullam ac euismod tortor, ac interdum velit. Aliquam eget laoreet orci, eu volutpat odio. Donec varius ligula id nibh elementum, eget maximus lorem sollicitudin. Interdum et malesuada fames ac ante ipsum primis in faucibus. Quisque in felis sit amet diam hendrerit faucibus. Nullam pharetra pretium dolor, at sagittis purus. Mauris et lectus iaculis, egestas ligula non, fermentum sapien. Nulla nec laoreet metus, sit amet condimentum leo. Sed a elit vitae orci hendrerit mollis at quis elit. Fusce faucibus augue vitae purus ultricies, id molestie eros gravida.

Aliquam aliquam orci magna, in ullamcorper diam viverra vel. Proin ultrices, enim eget vestibulum sodales, felis massa suscipit erat, ac dapibus nisi dolor vel ex. Ut elit orci, ultrices in consequat a, venenatis et metus. Sed nec lectus non ligula viverra aliquet. Nullam consequat neque et orci scelerisque, ornare consectetur dolor porttitor. Proin sit amet lacinia ligula. Ut nec quam elementum lacus tempor interdum et a nunc. Aenean congue augue quis varius sollicitudin. Vivamus sed volutpat tortor.

Maecenas a tellus congue, luctus nisi in, efficitur urna. Donec tellus massa, pharetra a quam dictum, volutpat dignissim nisi. Praesent est eros, molestie eget massa ut, iaculis accumsan erat. Nullam cursus, nisl a luctus mollis, diam ante imperdiet ex, ac finibus dolor velit in diam. Cras sed dolor a leo placerat viverra. Morbi tincidunt non purus sit amet tempus. Nam sed tincidunt ligula.)";

	std::string make_mixed_case()
	{
		std::string mixed_case( lorem_ipsum );
		std::mt19937_64 gen( 42 );
		std::uniform_int_distribution<int> dist( 0, 5 );
		for ( char& c : mixed_case )
		{
			if ( dist( gen ) == 0 )
			{
				c = mclo::to_upper( c );
			}
		}
		return mixed_case;
	}

	void BM_CompareIgnoreCaseScalar( benchmark::State& state )
	{
		const std::string mixed_case = make_mixed_case();
		for ( auto _ : state )
		{
			int result =
				mclo::detail::compare_ignore_case_scalar( lorem_ipsum.data(), mixed_case.data(), mixed_case.size() );
			benchmark::DoNotOptimize( result );
		}
	}
	BENCHMARK( BM_CompareIgnoreCaseScalar );

	void BM_CompareIgnoreCaseSimd( benchmark::State& state )
	{
		const std::string mixed_case = make_mixed_case();
		for ( auto _ : state )
		{
			int result =
				mclo::detail::compare_ignore_case_simd( lorem_ipsum.data(), mixed_case.data(), mixed_case.size() );
			benchmark::DoNotOptimize( result );
		}
	}
	BENCHMARK( BM_CompareIgnoreCaseSimd );

	void BM_ToUpperScalar( benchmark::State& state )
	{
		const std::string mixed_case = make_mixed_case();
		for ( auto _ : state )
		{
			std::string string = mixed_case;
			auto ptr = string.data();
			benchmark::DoNotOptimize( ptr );
			mclo::detail::to_upper_scalar( string.data(), string.data() + string.size() );
			benchmark::ClobberMemory();
		}
	}
	BENCHMARK( BM_ToUpperScalar );

	void BM_ToUpperSimd( benchmark::State& state )
	{
		const std::string mixed_case = make_mixed_case();
		for ( auto _ : state )
		{
			std::string string = mixed_case;
			auto ptr = string.data();
			benchmark::DoNotOptimize( ptr );
			mclo::detail::to_upper_simd( string.data(), string.data() + string.size() );
			benchmark::ClobberMemory();
		}
	}
	BENCHMARK( BM_ToUpperSimd );

	void BM_ToLowerScalar( benchmark::State& state )
	{
		const std::string mixed_case = make_mixed_case();
		for ( auto _ : state )
		{
			std::string string = mixed_case;
			auto ptr = string.data();
			benchmark::DoNotOptimize( ptr );
			mclo::detail::to_lower_scalar( string.data(), string.data() + string.size() );
			benchmark::ClobberMemory();
		}
	}
	BENCHMARK( BM_ToLowerScalar );

	void BM_ToLowerSimd( benchmark::State& state )
	{
		const std::string mixed_case = make_mixed_case();
		for ( auto _ : state )
		{
			std::string string = mixed_case;
			auto ptr = string.data();
			benchmark::DoNotOptimize( ptr );
			mclo::detail::to_lower_simd( string.data(), string.data() + string.size() );
			benchmark::ClobberMemory();
		}
	}
	BENCHMARK( BM_ToLowerSimd );

	void BM_ConcatStringOperatorPlus( benchmark::State& state )
	{
		for ( auto _ : state )
		{
			std::string string;
			string.reserve( 32 );
			auto ptr = string.data();
			benchmark::DoNotOptimize( ptr );
			string = std::string( "hello world I am a" ) + "pretty big string" + std::to_string( 42 ) + "got nums" +
					 "and stuff";
			benchmark::ClobberMemory();
		}
	}
	BENCHMARK( BM_ConcatStringOperatorPlus );

	void BM_ConcatStringOperatorPlusEquals( benchmark::State& state )
	{
		for ( auto _ : state )
		{
			std::string string;
			string.reserve( 32 );
			auto ptr = string.data();
			benchmark::DoNotOptimize( ptr );
			string += "hello world I am a";
			string += "pretty big string";
			string += std::to_string( 42 );
			string += "got nums";
			string += "and stuff";
			benchmark::ClobberMemory();
		}
	}
	BENCHMARK( BM_ConcatStringOperatorPlusEquals );

	void BM_ConcatStringConcat( benchmark::State& state )
	{
		for ( auto _ : state )
		{
			std::string str;
			auto ptr = str.data();
			benchmark::DoNotOptimize( ptr );
			mclo::append_string( str, "hello world I am a", "pretty big string", 42, "got nums", "and stuff" );
			benchmark::ClobberMemory();
		}
	}
	BENCHMARK( BM_ConcatStringConcat );
}
