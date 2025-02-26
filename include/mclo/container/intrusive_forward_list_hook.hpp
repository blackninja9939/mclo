#pragma once

namespace mclo
{
	template <typename Tag = void>
	class intrusive_forward_list_hook
	{
	public:
		template <typename, typename>
		friend class intrusive_forward_list;

		template <typename, typename>
		friend class atomic_intrusive_forward_list;

		template <typename, typename>
		friend class intrusive_forward_list_iterator;

		constexpr intrusive_forward_list_hook() noexcept = default;

		constexpr intrusive_forward_list_hook( const intrusive_forward_list_hook& ) noexcept
		{
		}

		constexpr intrusive_forward_list_hook& operator=( const intrusive_forward_list_hook& ) noexcept
		{
			return *this;
		}

	private:
		intrusive_forward_list_hook* m_next = nullptr;
	};
}
