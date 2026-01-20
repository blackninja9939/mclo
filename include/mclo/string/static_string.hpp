#pragma once

#include <string_view>

namespace mclo
{
	/// @brief Tag type to indicate that a string view is assumed to be static for the lifetime of the program.
	struct assume_static_t
	{
		explicit assume_static_t() = default;
	};

	/// @brief Tag instance to indicate that a string view is assumed to be static for the lifetime of the program.
	inline constexpr assume_static_t assume_static{};

	/// @brief A string type that can be constructed at compile time from static strings.
	/// @tparam CharT The character type.
	/// @tparam Traits The character traits type.
	/// @details This type is a thin wrapper around std::basic_string_view that allows for guaranteeing that the
	/// lifetime of the string is program wide. It can implicitly be constructed at compile time from string literals
	/// and string views. At run time, the user can explicitly state that a string view is assumed to be static for the
	/// lifetime of the program using the assume_static tag.
	/// @warning It is the user's responsibility to ensure that any string views passed with the assume_static tag are
	/// valid for the lifetime of the program.
	template <typename CharT, typename Traits = std::char_traits<CharT>>
	class basic_static_string
	{
	public:
		using view_type = std::basic_string_view<CharT, Traits>;

		using traits_type = typename view_type::traits_type;
		using value_type = typename view_type::value_type;
		using pointer = typename view_type::pointer;
		using const_pointer = typename view_type::const_pointer;
		using reference = typename view_type::reference;
		using const_reference = typename view_type::const_reference;
		using size_type = typename view_type::size_type;
		using difference_type = typename view_type::difference_type;

		constexpr basic_static_string() = default;

		consteval basic_static_string( const view_type str ) noexcept
			: m_str( str )
		{
		}

		consteval basic_static_string( const_pointer str ) noexcept
			: m_str( str )
		{
		}

		consteval basic_static_string( const_pointer str, const size_type len ) noexcept
			: m_str( str, len )
		{
		}

		template <size_type N>
		consteval basic_static_string( const value_type ( &str )[ N ] ) noexcept
			: m_str( str, N - 1 )
		{
		}

		constexpr basic_static_string( assume_static_t, const view_type str ) noexcept
			: m_str( str )
		{
		}

		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return m_str.size();
		}

		[[nodiscard]] constexpr const_pointer data() const noexcept
		{
			return m_str.data();
		}

		[[nodiscard]] constexpr operator view_type() const noexcept
		{
			return m_str;
		}

		friend const view_type& format_as( const basic_static_string& str ) noexcept
		{
			return str.m_str;
		}

		constexpr auto operator<=>( const basic_static_string& rhs ) const = default;

	private:
		view_type m_str;
	};

	using static_string = basic_static_string<char>;
	using static_wstring = basic_static_string<wchar_t>;
	using static_u8string = basic_static_string<char8_t>;
	using static_u16string = basic_static_string<char16_t>;
	using static_u32string = basic_static_string<char32_t>;
}

template <typename CharT, typename Traits>
struct std::hash<mclo::basic_static_string<CharT, Traits>>
{
	using view_type = typename mclo::basic_static_string<CharT, Traits>::view_type;

	[[nodiscard]] std::size_t operator()( const mclo::basic_static_string<CharT, Traits>& str ) const noexcept
	{
		return std::hash<view_type>{}( static_cast<view_type>( str ) );
	}
};
