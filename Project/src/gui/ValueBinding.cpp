#include <pch.h>
#include "gui/ValueBinding.h"

namespace fig::gui
{
	void ValueBinding<int32_t>::Set(fig::string value) noexcept
	{
		(*_ptr) = string_to_int(value, 0);
	}

	fig::string ValueBinding<int32_t>::AsString() const noexcept
	{
		return int_to_string(*_ptr);
	}

	void ValueBinding<float>::Set(fig::string value) noexcept
	{
		(*_ptr) = string_to_float(value, 0.0f);
	}

	fig::string ValueBinding<float>::AsString() const noexcept
	{
		return float_to_string(*_ptr);
	}

	void ValueBinding<fig::fixed>::Set(fig::string value) noexcept
	{
		(*_ptr) = string_to_fixed(value, 0_fp);
	}

	fig::string ValueBinding<fig::fixed>::AsString() const noexcept
	{
		return fixed_to_string(*_ptr);
	}

}