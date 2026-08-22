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
		void Set(int32_t value) noexcept = delete;
		int32_t AsInt() const noexcept = delete;

		void Set(const T& value) noexcept
		{
			(*_ptr) = value;
		}

		const T& Get() const noexcept
		{
			return *_ptr;
		}

		T* _ptr;
	};

	template <typename T>
	concept is_string_value_bindable = is_string_convertible<T>;
	template <typename T>
	concept is_int_value_bindable = std::is_enum_v<T> and (not is_string_convertible<T>);

	template <is_string_value_bindable T>
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

	template <is_int_value_bindable T>
	struct ValueBinding<T>
	{
		ValueBinding() = delete;
		ValueBinding(const ValueBinding& other) : _ptr { other._ptr } {}
		explicit ValueBinding(T* ptr) : _ptr { ptr } {}

		void Set(int32_t value) noexcept
		{
			(*_ptr) = T { value };
		}

		int32_t AsInt() const noexcept
		{
			return static_cast<int32_t>(*_ptr);
		}

		T* _ptr;
	};
}