#pragma once

#include <optional>

namespace fig
{
	template<typename T>
	class optional_ref
	{
	public:
		optional_ref() = default;
		optional_ref(std::nullopt_t) noexcept {}
		optional_ref(T& value) noexcept : _pointer(&value) {}

		bool has_value() const noexcept { return _pointer != nullptr; }
		explicit operator bool() const noexcept { return has_value(); }

		T& value() const
		{
			if (not _pointer)
				throw std::bad_optional_access {};
			return *_pointer;
		}

		T& operator*() const noexcept { return *_pointer; }
		T* operator->() const noexcept { return _pointer; }

		T& value_or(T& fallback) const noexcept { return _pointer ? *_pointer : fallback; }

		void reset() noexcept { _pointer = nullptr; }

		template<typename F>
		auto transform(F&& function) const -> std::optional<std::invoke_result_t<F, T&>>
		{
			if (not _pointer)
				return std::nullopt;
			return std::invoke(std::forward<F>(function), *_pointer);
		}

		template<typename F>
			requires std::default_initializable<std::invoke_result_t<F, T&>>
		auto and_then(F&& function) const -> std::invoke_result_t<F, T&>
		{
			if (not _pointer)
				return {};
			return std::invoke(std::forward<F>(function), *_pointer);
		}

		template<typename F>
		auto or_else(F&& function) const -> optional_ref<T>
		{
			if (_pointer)
				return *this;
			return std::invoke(std::forward<F>(function));
		}

	private:
		T* _pointer = nullptr;
	};

	template <typename T>
	using optional_cref = optional_ref<const T>;

	template <typename T>
	[[nodiscard]] constexpr optional_ref<T> make_optional_ref(T& value) noexcept
	{
		return optional_ref<T>(value);
	}

	template <typename T>
	[[nodiscard]] constexpr optional_cref<T> make_optional_cref(const T& value) noexcept
	{
		return optional_cref<T>(value);
	}

	using nullopt_t = std::nullopt_t;
	constexpr nullopt_t nullref = std::nullopt;
}
