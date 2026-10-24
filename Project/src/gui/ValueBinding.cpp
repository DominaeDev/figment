#include <pch.h>
#include "gui/ValueBinding.h"

namespace fig::gui
{
	template<>
	void ValueBinding<int32_t>::Set(fig::string_view value) noexcept
	{
		(*_ptr) = string_to_int(value, 0);
	}

	template<>
	fig::string ValueBinding<int32_t>::AsString() const noexcept
	{
		return int_to_string(*_ptr);
	}

	template<>
	void ValueBinding<float>::Set(fig::string_view value) noexcept
	{
		(*_ptr) = string_to_float(value, 0.0f);
	}

	template<>
	fig::string ValueBinding<float>::AsString() const noexcept
	{
		return float_to_string(*_ptr);
	}

	template<>
	void ValueBinding<fig::fixed>::Set(fig::string_view value) noexcept
	{
		(*_ptr) = string_to_fixed(value, 0_fp);
	}

	template<>
	fig::string ValueBinding<fig::fixed>::AsString() const noexcept
	{
		return fixed_to_string(*_ptr);
	}



}