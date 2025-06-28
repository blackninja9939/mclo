#include <type_traits>
#include <utility>

namespace mclo
{
	template <typename Factory>
	class lazy_convert_construct
	{
	public:
		using result_type = std::invoke_result_t<const Factory&>;

		constexpr lazy_convert_construct( Factory&& factory ) noexcept( std::is_nothrow_move_constructible_v<factory> )
			: m_factory( std::move( factory ) )
		{
		}

		[[nodiscard]] constexpr operator result_type() const noexcept( std::is_nothrow_invocable_v<const Factory&> )
		{
			return m_factory();
		}

	private:
		Factory m_factory;
	};
}
