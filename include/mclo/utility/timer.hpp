#pragma once

#include <chrono>

namespace mclo
{
	/// @brief Measures elapsed wall clock time from a recorded reference point.
	/// @details Records a time point on construction and reports the duration since then. Use @ref elapsed to
	/// read the time without resetting, @ref tick to read and restart in one step, or @ref reset to restart.
	/// @tparam Clock The clock type used to take measurements.
	/// @tparam Duration The duration type that elapsed times are reported in.
	template <typename Clock = std::chrono::high_resolution_clock, typename Duration = typename Clock::duration>
	class timer
	{
		using time_point = typename Clock::time_point;

	public:
		/// @brief The clock type used to take measurements.
		using clock = Clock;

		/// @brief The duration type elapsed times are reported in.
		using duration = Duration;

		/// @brief Returns the time elapsed since the reference point without modifying it.
		/// @return The duration since the timer was last reset or constructed.
		duration elapsed() const noexcept
		{
			const time_point now = clock::now();
			return std::chrono::duration_cast<duration>( now - m_point );
		}

		/// @brief Returns the time elapsed since the reference point and resets it to now.
		/// @return The duration since the timer was last reset, ticked, or constructed.
		duration tick() noexcept
		{
			const time_point now = clock::now();
			const duration delta = std::chrono::duration_cast<duration>( now - m_point );
			m_point = now;
			return delta;
		}

		/// @brief Resets the reference point to the current time.
		void reset() noexcept
		{
			m_point = clock::now();
		}

	private:
		time_point m_point = clock::now();
	};
}
