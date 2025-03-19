#pragma once

#include "mclo/hash/hash_append.hpp"

#include <chrono>

namespace mclo
{
	template <mclo::hasher Hasher, typename Rep, typename Period>
	void hash_append( Hasher& hasher, const std::chrono::duration<Rep, Period>& value ) noexcept
	{
		hash_append( hasher, value.count() );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::chrono::day& value ) noexcept
	{
		hash_append( hasher, value.operator unsigned int() );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::chrono::month& value ) noexcept
	{
		hash_append( hasher, value.operator unsigned int() );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::chrono::year& value ) noexcept
	{
		hash_append( hasher, value.operator int() );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::chrono::weekday& value ) noexcept
	{
		hash_append( hasher, value.c_encoding() );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::chrono::weekday& value ) noexcept
	{
		hash_append( hasher, value.c_encoding() );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::chrono::weekday_indexed& value ) noexcept
	{
		hash_append( hasher, value.weekday() );
		hash_append( hasher, value.index() );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::chrono::weekday_last& value ) noexcept
	{
		hash_append( hasher, value.weekday() );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::chrono::month_day_last& value ) noexcept
	{
		hash_append( hasher, value.month() );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::chrono::month_weekday& value ) noexcept
	{
		hash_append( hasher, value.month() );
		hash_append( hasher, value.weekday_indexed() );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::chrono::month_weekday_last& value ) noexcept
	{
		hash_append( hasher, value.month() );
		hash_append( hasher, value.weekday_last() );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::chrono::year_month& value ) noexcept
	{
		hash_append( hasher, value.year() );
		hash_append( hasher, value.month() );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::chrono::year_month_day& value ) noexcept
	{
		hash_append( hasher, value.year() );
		hash_append( hasher, value.month() );
		hash_append( hasher, value.day() );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::chrono::year_month_day_last& value ) noexcept
	{
		hash_append( hasher, value.year() );
		hash_append( hasher, value.month_day_last() );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::chrono::year_month_weekday& value ) noexcept
	{
		hash_append( hasher, value.year() );
		hash_append( hasher, value.month() );
		hash_append( hasher, value.weekday_indexed() );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::chrono::year_month_weekday_last& value ) noexcept
	{
		hash_append( hasher, value.year() );
		hash_append( hasher, value.month() );
		hash_append( hasher, value.weekday_last() );
	}

	template <mclo::hasher Hasher, typename Duration, typename TimeZonePtr>
	void hash_append( Hasher& hasher, const std::chrono::zoned_time<Duration, TimeZonePtr>& value ) noexcept
	{
		hash_append( hasher, value.get_time_zone() );
		hash_append( hasher, value.get_sys_time() );
	}
}
