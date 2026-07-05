#pragma once

#include <expected>

namespace fig
{
	template<typename T, typename E>
	class expected_ref
	{
	public:
		expected_ref(T& value) noexcept : _inner(&value) {}
		expected_ref(std::unexpected<E> error) : _inner(std::move(error)) {}

		bool has_value() const noexcept { return _inner.has_value(); }
		explicit operator bool() const noexcept { return has_value(); }

		T& value() const
		{
			if (not _inner.has_value())
				throw std::bad_expected_access<E>(_inner.error());
			return **_inner;
		}

		E& error() noexcept { return _inner.error(); }
		const E& error() const noexcept { return _inner.error(); }

		T& operator*() const noexcept { return **_inner; }
		T* operator->() const noexcept { return *_inner; }

		T& value_or(T& fallback) const noexcept { return has_value() ? **_inner : fallback; }

		template<typename F>
		auto transform(F&& function) const -> std::expected<std::invoke_result_t<F, T&>, E>
		{
			if (not has_value())
				return std::unexpected(_inner.error());
			return std::invoke(std::forward<F>(function), **_inner);
		}

		template<typename F>
			requires std::constructible_from<std::invoke_result_t<F, T&>, std::unexpected<E>>
		auto and_then(F&& function) const -> std::invoke_result_t<F, T&>
		{
			if (not has_value())
				return std::unexpected(_inner.error());
			return std::invoke(std::forward<F>(function), **_inner);
		}

		template<typename F>
			requires std::constructible_from<std::invoke_result_t<F, E&>, T&>
		auto or_else(F&& function) const -> std::invoke_result_t<F, E&>
		{
			using ReturnType = std::invoke_result_t<F, E&>;
			if (has_value())
				return ReturnType(**_inner);
			return std::invoke(std::forward<F>(function), _inner.error());
		}

		template<typename F>
		auto transform_error(F&& function) const -> expected_ref<T, std::invoke_result_t<F, E&>>
		{
			if (has_value())
				return **_inner;
			return std::unexpected(std::invoke(std::forward<F>(function), _inner.error()));
		}

		operator expected_ref<const T, E>() const
		{
			if (has_value())
				return **_inner;
			return std::unexpected(_inner.error());
		}

	private:
		std::expected<T*, E> _inner;
	};

	template <typename T, typename E>
	using expected_cref = expected_ref<const T, E>;

	template <typename E>
	std::unexpected<std::remove_cvref_t<E>> unexpected(E&& error)
	{
		return std::unexpected<std::remove_cvref_t<E>>(std::forward<E>(error));
	}
}