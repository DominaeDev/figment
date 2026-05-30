#include <pch.h>
#include "data/Contextual.h"

namespace fig::data
{
	template<>
	void Contextual::SetValue<int32_t>(fig::handle name, int32_t value) noexcept
	{
		_values[std::move(name)] = std::move(value);
	}

	template<>
	void Contextual::SetValue<float>(fig::handle name, float value) noexcept
	{
		_values[std::move(name)] = std::move(value);
	}

	template<>
	void Contextual::SetValue<fig::string>(fig::handle name, fig::string value) noexcept
	{
		_values[std::move(name)] = std::move(value);
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
		if (_flags.contains(name))
			return true;

		if (auto itFind = _values.find(name); itFind != _values.cend())
		{
			auto& var_value = itFind->second;
			if (auto value = std::get_if<int32_t>(&var_value))
				return (*value) != 0;
			if (auto value = std::get_if<float>(&var_value))
				return not flt_eq(*value, 0.0f);
			if (auto value = std::get_if<fig::string>(&var_value))
				return not (*value).empty();
		}
		return false;
	}

	void Contextual::AddContext(fig::handle name, Contextual& other) noexcept
	{
		_contexts.emplace(name, std::ref(other));
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

	std::optional<ContextualRef> Contextual::GetContext(fig::handle name) noexcept
	{
		if (auto itFind = _contexts.find(name); itFind != _contexts.cend())
			return std::make_optional(itFind->second);
			return std::nullopt;
	}
}