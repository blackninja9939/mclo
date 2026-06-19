#pragma once

namespace mclo
{
	/// @brief Parameterised mixin that adds a subscript operator taking an index of type Index.
	/// @details Supply the parameterised tag directly, e.g. mclo::strong_typedef<std::array<int, 4>, struct tag,
	/// mclo::indexed<std::size_t>>.
	/// @tparam Index The index type accepted by the generated subscript operator.
	template <typename Index>
	struct indexed
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] constexpr decltype( auto ) operator[]( const Index& index ) const
			{
				return static_cast<const Derived&>( *this ).value[ index ];
			}
			[[nodiscard]] constexpr decltype( auto ) operator[]( const Index& index )
			{
				return static_cast<Derived&>( *this ).value[ index ];
			}
		};
	};
}
