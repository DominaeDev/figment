#include <pch.h>
#include "text/Context.h"

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

	Selector Selector::Append(const fig::string& key) const noexcept
	{
		if (not key.empty())
		{
			Selector selector {};
			selector._value = _value;
			selector._value.push_back(key);
			return selector;
		}
		return *this;
	}

	template<>
	void Context::SetValue<int32_t>(fig::handle name, const int32_t& value) noexcept
	{
		_values[std::move(name)] = value;
	}

	template<>
	void Context::SetValue<float>(fig::handle name, const float& value) noexcept
	{
		_values[std::move(name)] = value;
	}

	template<>
	void Context::SetValue<fig::string>(fig::handle name, const fig::string& value) noexcept
	{
		_values[std::move(name)] = value;
	}

	template<>
	[[nodiscard]] std::optional<bool> Context::TryGetValue_Internal<bool>(fig::handle name) const noexcept
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
	[[nodiscard]] std::optional<int32_t> Context::TryGetValue_Internal<int32_t>(fig::handle name) const noexcept
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
	[[nodiscard]] std::optional<float> Context::TryGetValue_Internal<float>(fig::handle name) const noexcept
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
	[[nodiscard]] std::optional<fig::string> Context::TryGetValue_Internal<fig::string>(fig::handle name) const noexcept
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

	bool Context::HasValue(fig::handle name) const noexcept
	{
		return _values.contains(name);
	}

	void Context::ClearValues() noexcept
	{
		_values.clear();
	}

	void Context::SetFlag(fig::handle flag) noexcept
	{
		_flags.insert(flag);
	}

	void Context::SetFlags(std::span<fig::handle> flags) noexcept
	{
		_flags.insert_range(flags);
	}

	void Context::UnsetFlag(fig::handle flag) noexcept
	{
		_flags.erase(flag);
	}

	void Context::UnsetFlags(std::span<fig::handle> flags) noexcept
	{
		for (auto& f : flags)
			_flags.erase(f);
	}

	bool Context::HasFlag(fig::handle flag) const noexcept
	{
		return _flags.contains(flag);
	}

	void Context::ClearFlags() noexcept
	{
		_flags.clear();
	}

	bool Context::operator[](ContextLocation location) const noexcept
	{
		if (location.selector.empty() and not location.key.empty())
		{
			if (auto itAlias = _valueAliases.find(location.key); itAlias != _contextAliases.cend())
				return GetBool_Internal(itAlias->second.key);
		}

		return GetBool_Internal(location.key);
	}

	bool Context::GetBool_Internal(ContextLocation location) const noexcept
	{
		if (auto try_ctx = TryGetContext(location.selector))
		{
			auto& ctx = try_ctx.value().get();
			return ctx.GetBool_Internal(location.key);
		}
		return false;
	}

	bool Context::GetBool_Internal(fig::handle name) const noexcept
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

	Context& Context::AddContext(fig::handle name) noexcept
	{
		_contexts.emplace(name, Context {});
		return _contexts.at(name);
	}

	Context& Context::AddContext(fig::handle name, const Context& context) noexcept
	{
		auto [it, _] = _contexts.insert_or_assign(name, context);
		return it->second;
	}

	Context& Context::AddContext(fig::handle name, Context&& context) noexcept
	{
		auto [it, _] = _contexts.insert_or_assign(name, std::move(context));
		return it->second;
	}

	bool Context::RemoveContext(fig::handle name) noexcept
	{
		if (auto itFind = _contexts.find(name); itFind != _contexts.end())
		{
			_contexts.erase(itFind);
			return true;
		}
		return false;
	}

	std::optional<ContextualRef> Context::TryGetContext(fig::handle name) noexcept
	{
		if (name.empty())
			return std::nullopt;

		if (auto itFind = _contexts.find(name); itFind != _contexts.end())
			return std::make_optional(std::ref(itFind->second));
		return std::nullopt;
	}

	std::optional<ContextualCRef> Context::TryGetContext(fig::handle name) const noexcept
	{
		if (auto itAlias = _contextAliases.find(name); itAlias != _contextAliases.cend())
			return TryGetContext((Selector)itAlias->second);

		if (name.empty())
			return std::nullopt;

		if (auto itFind = _contexts.find(name); itFind != _contexts.cend())
			return std::make_optional(std::cref(itFind->second));
		return std::nullopt;
	}

	std::optional<ContextualRef> Context::TryGetContext(Selector selector) noexcept
	{
		// Resolve alias
		if (selector.GetKeys().size() == 1)
		{
			auto& key = selector.GetKeys()[0];
			if (auto itAlias = _contextAliases.find(key); itAlias != _contextAliases.cend())
				return TryGetContext(itAlias->second);
		}

		if (not selector.empty())
			return TryGetContext(ContextLocation { selector });
		return std::ref(*this);
	}

	std::optional<ContextualCRef> Context::TryGetContext(Selector selector) const noexcept
	{
		// Resolve alias
		if (selector.GetKeys().size() == 1)
		{
			auto& key = selector.GetKeys()[0];
			if (auto itAlias = _contextAliases.find(key); itAlias != _contextAliases.cend())
				return TryGetContext(itAlias->second);
		}

		if (not selector.empty())
			return TryGetContext(ContextLocation { selector });
		return std::cref(*this);
	}

	std::optional<ContextualRef> Context::TryGetContext(ContextLocation location) noexcept
	{
		auto selector = (Selector)location;
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

	std::optional<ContextualCRef> Context::TryGetContext(ContextLocation location) const noexcept
	{
		auto selector = (Selector)location;
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

	void Context::Clear() noexcept
	{
		_values.clear();
		_flags.clear();
		_contexts.clear();
		_valueAliases.clear();
		_contextAliases.clear();
	}

	void Context::AddValueAlias(fig::handle alias, const ContextLocation& target) noexcept
	{
		_valueAliases.insert_or_assign(alias, target);
	}

	void Context::AddContextAlias(fig::handle alias, const ContextLocation& target) noexcept
	{
		_contextAliases.insert_or_assign(alias, target);
	}

	void Context::RemoveAlias(fig::handle alias) noexcept
	{
		_valueAliases.erase(alias);
		_contextAliases.erase(alias);
	}
}