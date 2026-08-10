#pragma once

#include "Figment.h"

namespace fig::gui
{
	template <typename T>
	struct ValueBinding
	{
		ValueBinding() = delete;
		ValueBinding(const ValueBinding& other) : _ptr { other._ptr } {}
		explicit ValueBinding(T* ptr) : _ptr { ptr } {}

		void Set(fig::string value) noexcept = delete;
		fig::string AsString() const noexcept = delete;

		T* _ptr;
	};

	template <is_string_convertible T>
	struct ValueBinding<T>
	{
		ValueBinding() = delete;
		ValueBinding(const ValueBinding& other) : _ptr { other._ptr } {}
		explicit ValueBinding(T* ptr) : _ptr { ptr } {}

		void Set(fig::string value) noexcept
		{
			(*_ptr) = value;
		}

		fig::string AsString() const noexcept
		{
			return (fig::string)(*_ptr);
		}

		T* _ptr;
	};
}