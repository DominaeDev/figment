#include <pch.h>
#include "text/Contextual.h"

namespace fig
{
	Selector::Selector(const fig::string& input)
	{
		_value.assign_range(input
			| std::ranges::views::split('.')
			| std::views::transform([](auto range) -> fig::handle {
				return fig::handle { std::string_view(range.data(), range.size()) };
			}));

		const auto [first, last] = std::ranges::remove_if(_value, [](auto& h) { return h.empty(); });
		_value.erase(first, last);
	}

	template<>
	void Contextual::SetValue<int32_t>(fig::handle name, const int32_t& value) noexcept
	{
		_values[std::move(name)] = value;
	}

	template<>
	void Contextual::SetValue<float>(fig::handle name, const float& value) noexcept
	{
		_values[std::move(name)] = value;
	}

	template<>
	void Contextual::SetValue<fig::string>(fig::handle name, const fig::string& value) noexcept
	{
		_values[std::move(name)] = value;
	}

	template<>
	[[nodiscard]] std::optional<bool> Contextual::TryGetValue<bool>(fig::handle name) const noexcept
	{
		if (auto itFind = _values.find(name); itFind != _values.cend())
		{
			auto& var_value = itFind->second;
			if (auto value = std::get_if<int32_t>(&var_value))
				return (*value) != 0;
			else if (auto value = std::get_if<float>(&var_value))
				return not flt_eq(*value, 0.0f);
			else if (auto value = std::get_if<fig::string>(&var_value))
				return not (*value).empty();
		}

		return std::nullopt;
	}

	template<>
	[[nodiscard]] std::optional<int32_t> Contextual::TryGetValue<int32_t>(fig::handle name) const noexcept
	{
		if (auto itFind = _values.find(name); itFind != _values.cend())
		{
			auto& var_value = itFind->second;
			if (auto value = std::get_if<int32_t>(&var_value))
				return *value;
			if (auto value = std::get_if<float>(&var_value))
				return toI(*value);
			if (auto value = std::get_if<fig::string>(&var_value))
				return string_to_int(*value, 0);
		}

		return std::nullopt;
	}

	template<>
	[[nodiscard]] std::optional<float> Contextual::TryGetValue<float>(fig::handle name) const noexcept
	{
		if (auto itFind = _values.find(name); itFind != _values.cend())
		{
			auto& var_value = itFind->second;
			if (auto value = std::get_if<int32_t>(&var_value))
				return toF(*value);
			if (auto value = std::get_if<float>(&var_value))
				return *value;
			if (auto value = std::get_if<fig::string>(&var_value))
				return string_to_float(*value, 0.0f);
		}

		return std::nullopt;
	}

	template<>
	[[nodiscard]] std::optional<fig::string> Contextual::TryGetValue<fig::string>(fig::handle name) const noexcept
	{
		if (auto itFind = _values.find(name); itFind != _values.cend())
		{
			auto& var_value = itFind->second;
			if (auto value = std::get_if<int32_t>(&var_value))
				return int_to_string(*value);
			if (auto value = std::get_if<float>(&var_value))
				return float_to_string(*value);
			if (auto value = std::get_if<fig::string>(&var_value))
				return *value;
		}

		return std::nullopt;
	}

	bool Contextual::HasValue(fig::handle name) const noexcept
	{
		return _values.contains(name);
	}

	void Contextual::ClearValues() noexcept
	{
		_values.clear();
	}

	void Contextual::SetFlag(fig::handle flag) noexcept
	{
		_flags.insert(flag);
	}

	void Contextual::SetFlags(std::span<fig::handle> flags) noexcept
	{
		_flags.insert_range(flags);
	}

	void Contextual::UnsetFlag(fig::handle flag) noexcept
	{
		_flags.erase(flag);
	}

	void Contextual::UnsetFlags(std::span<fig::handle> flags) noexcept
	{
		for (auto& f : flags)
			_flags.erase(f);
	}

	bool Contextual::HasFlag(fig::handle flag) const noexcept
	{
		return _flags.contains(flag);
	}

	void Contextual::ClearFlags() noexcept
	{
		_flags.clear();
	}

	bool Contextual::operator[](fig::handle name) const noexcept
	{
		// Check flags
		if (_flags.contains(name))
			return true;

		// Check values
		if (auto try_bool = TryGetValue<bool>(name))
			return try_bool.value();

		// Check contexts
		if (_contexts.contains(name))
			return true;
		return false;
	}

	Contextual& Contextual::AddContext(fig::handle name) noexcept
	{
		_contexts.emplace(name, Contextual {});
		return _contexts.at(name);
	}

	bool Contextual::RemoveContext(fig::handle name) noexcept
	{
		if (auto itFind = _contexts.find(name); itFind != _contexts.end())
		{
			_contexts.erase(itFind);
			return true;
		}
		return false;
	}

	std::optional<ContextualRef> Contextual::TryGetContext(fig::handle name) noexcept
	{
		if (name.empty())
			return std::nullopt;

		if (auto itFind = _contexts.find(name); itFind != _contexts.end())
			return std::make_optional(std::ref(itFind->second));
		return std::nullopt;
	}

	std::optional<ContextualCRef> Contextual::TryGetContext(fig::handle name) const noexcept
	{
		if (name.empty())
			return std::nullopt;

		if (auto itFind = _contexts.find(name); itFind != _contexts.cend())
			return std::make_optional(std::cref(itFind->second));
		return std::nullopt;
	}

	std::optional<ContextualRef> Contextual::TryGetContext(Selector selector) noexcept
	{
		if (selector.empty())
			return std::ref(*this);

		auto pCtx = this;
		auto keys = selector.GetKeys();
		for (auto& key : keys)
		{
			if (auto itNext = pCtx->TryGetContext(key))
			{
				pCtx = &itNext.value().get();
				continue;
			}
			else
				return std::nullopt;
		}
		return std::ref(*pCtx);
	}

	std::optional<ContextualCRef> Contextual::TryGetContext(Selector selector) const noexcept
	{
		if (selector.empty())
			return std::ref(*this);

		auto pCtx = this;
		auto keys = selector.GetKeys();
		for (auto& key : keys)
		{
			if (auto itNext = pCtx->TryGetContext(key))
			{
				pCtx = &itNext.value().get();
				continue;
			}
			else
				return std::nullopt;
		}
		return std::cref(*pCtx);
	}

	Contextual Contextual::GetContext() const noexcept
	{
		return Contextual { *this };
	}
}