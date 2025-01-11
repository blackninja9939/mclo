#pragma once

#include <atomic>

namespace mclo
{
	template <typename Derived>
	class intrusive_ref_counter
	{
	public:
		intrusive_ref_counter() noexcept = default;

		intrusive_ref_counter( const intrusive_ref_counter& ) noexcept
		{
		}

		intrusive_ref_counter& operator=( const intrusive_ref_counter& ) noexcept
		{
			return *this;
		}

		std::size_t use_count() const noexcept
		{
			return m_counter.load( std::memory_order_relaxed );
		}

		friend void intrusive_ptr_add_ref( const intrusive_ref_counter<Derived>* ptr ) noexcept
		{
			ptr->m_counter.fetch_add( 1, std::memory_order_relaxed );
		}

		friend void intrusive_ptr_release_ref( const intrusive_ref_counter<Derived>* ptr ) noexcept
		{
			const std::size_t old = ptr->m_counter.fetch_sub( 1, std::memory_order_acq_rel );
			if ( old == 1 ) // Was last reference
			{
				delete static_cast<const Derived*>( ptr );
			}
		}

	protected:
		~intrusive_ref_counter() = default;

	private:
		mutable std::atomic_size_t m_counter{ 0 };
	};
}
