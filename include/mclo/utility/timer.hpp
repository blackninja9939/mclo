#pragma once

#include <chrono>

namespace mclo
{
	template <typename Clock = std::chrono::high_resolution_clock, typename Duration = typename Clock::duration>
	class timer
	{
		using time_point = typename Clock::time_point;

	public:
		using clock = Clock;
		using duration = Duration;

		duration elapsed() const noexcept
		{
			const time_point now = clock::now();
			return std::chrono::duration_cast<duration>( now - m_point );
		}

		duration tick() noexcept
		{
			const time_point now = clock::now();
			const duration delta = std::chrono::duration_cast<duration>( now - m_point );
			m_point = now;
			return delta;
		}

		void reset() noexcept
		{
			m_point = clock::now();
		}

	private:
		time_point m_point = clock::now();
	};
}
