#include <pch.h>
#include "text/Context.h"
#include "text/TextEvaluator.h"
#include "text/Condition.h"
#include "text/MacroProvider.h"

namespace fig
{
	Context::Context()
	{
		_pCustomMacroProvider = std::make_unique<fig::text::MacroProvider>();
	}

	Context::Context(const Context& other)
	{
		operator=(other);
	}

	Context::Context(Context&& other) noexcept
	{
		operator=(std::move(other));
	}

	Context::~Context()
	{
	}

	Context& Context::operator=(const Context& other)
	{
		_flags = other._flags;
		_values = other._values;
		_contexts = other._contexts;
		_pGlobalMacroProvider = other._pGlobalMacroProvider;
		_pCustomMacroProvider = other._pCustomMacroProvider ? std::make_unique<fig::text::MacroProvider>(*other._pCustomMacroProvider) : nullptr;
		return *this;
	}

	Context& Context::operator=(Context&& other) noexcept
	{
		_flags = std::move(other._flags);
		_values = std::move(other._values);
		_contexts = std::move(other._contexts);
		_pGlobalMacroProvider = std::move(other._pGlobalMacroProvider);
		_pCustomMacroProvider = std::move(other._pCustomMacroProvider);
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

	void Context::RemoveValue(fig::handle name) noexcept
	{
		_values.erase(name);
	}

	[[nodiscard]] std::optional<ContextValue> Context::TryGetRaw_Internal(fig::handle name) const noexcept
	{
		if (auto itFind = _values.find(name); itFind != _values.cend())
			return itFind->second;
		
		if (_primarySelector)
		{
			if (auto primary_ctx = TryGetContext_Internal(_primarySelector); primary_ctx.has_value() and &primary_ctx.value() != this)
				return (*primary_ctx).TryGetRaw_Internal(name);
		}
		return std::nullopt;
	}

	template<>
	[[nodiscard]] std::optional<bool> Context::TryGetValue_Internal<bool>(fig::handle name) const noexcept
	{
		if (auto try_value = TryGetRaw_Internal(name))
		{
			auto& var_value = try_value.value();
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
		if (auto try_value = TryGetRaw_Internal(name))
		{
			auto& var_value = try_value.value();
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
		if (auto try_value = TryGetRaw_Internal(name))
		{
			auto& var_value = try_value.value();
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
		if (auto try_value = TryGetRaw_Internal(name))
		{
			auto& var_value = try_value.value();
			if (auto value = std::get_if<int32_t>(&var_value))
				return int_to_string(*value);
			if (auto value = std::get_if<float>(&var_value))
				return float_to_string(*value);
			if (auto value = std::get_if<fig::string>(&var_value))
				return *value;
		}

		return std::nullopt;
	}

	void Context::ClearValues() noexcept
	{
		_values.clear();
	}

	void Context::SetFlag(fig::handle flag) noexcept
	{
		_flags.insert(flag);
	}

	void Context::SetFlag(fig::handle flag, bool bEnabled) noexcept
	{
		if (bEnabled)
			_flags.insert(flag);
		else
			_flags.erase(flag);
	}

	void Context::SetFlags(const std::set<fig::handle>& flags) noexcept
	{
		_flags.insert_range(flags);
	}

	void Context::RemoveFlag(fig::handle flag) noexcept
	{
		_flags.erase(flag);
	}

	void Context::RemoveFlags(const std::set<fig::handle>& flags) noexcept
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

	void Context::ResolveAlias(ContextSelector& selector) const noexcept
	{
		if (auto pMacros = _pGlobalMacroProvider.lock())
		{
			if (pMacros->ApplyAlias(selector))
				return;
		}
		_pCustomMacroProvider->ApplyAlias(selector);
	}

	void Context::ResolveAlias(ContextLocator& location) const noexcept
	{
		if (auto pMacros = _pGlobalMacroProvider.lock())
		{
			if (pMacros->ApplyAlias(location))
				return;
		}
		_pCustomMacroProvider->ApplyAlias(location);
	}

	std::optional<const string_view> Context::TryGetMacro(const fig::handle& macro) const noexcept
	{
		if (auto pMacros = _pGlobalMacroProvider.lock())
		{
			if (auto try_macro = pMacros->TryGetMacro(macro))
				return try_macro;
		}
		if (auto try_macro = _pCustomMacroProvider->TryGetMacro(macro))
			return try_macro;
		return std::nullopt;
	}

	fig::optional_cref<Condition> Context::TryGetCondition(const fig::handle& alias) const noexcept
	{
		if (auto pMacros = _pGlobalMacroProvider.lock())
		{
			if (auto try_macro = pMacros->TryGetCondition(alias))
				return make_optional_cref(*try_macro);
		}
		if (auto try_macro = _pCustomMacroProvider->TryGetCondition(alias))
			return make_optional_cref(*try_macro);
		return fig::nullref;
	}

	bool Context::operator[](ContextLocator location) const noexcept
	{
		ResolveAlias(location);
		return GetBool_Internal(location.key);
	}

	bool Context::GetBool_Internal(ContextLocator location) const noexcept
	{
		if (auto try_ctx = TryGetContext(location.selector))
		{
			auto& ctx = try_ctx.value();
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

		// Check primary
		if (_primarySelector)
		{
			if (auto primary_ctx = TryGetContext_Internal(_primarySelector); primary_ctx.has_value() and &primary_ctx.value() != this)
				return (*primary_ctx).GetBool_Internal(name);
		}

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

	fig::optional_ref<Context> Context::TryGetContext(ContextSelector selector) noexcept
	{
		ResolveAlias(selector);
		if (not selector.empty())
			return TryGetContext_Internal(selector);
		return make_optional_ref(*this);
	}

	fig::optional_cref<Context> Context::TryGetContext(ContextSelector selector) const noexcept
	{
		ResolveAlias(selector);
		if (not selector.empty())
			return TryGetContext_Internal(selector);
		return make_optional_cref(*this);
	}

	fig::optional_ref<Context> Context::TryGetContext_Internal(ContextSelector selector) noexcept
	{
		if (selector.empty())
			return make_optional_ref(*this);

		auto pCtx = this;
		for (size_t i = 0; i < selector.size(); ++i)
		{
			auto& key = selector[i];
			if (auto itNext = pCtx->TryGetContext_Internal(key))
			{
				pCtx = &itNext.value();
				continue;
			}
			else
				return fig::nullref;
		}
		return make_optional_ref(*pCtx);
	}

	fig::optional_cref<Context> Context::TryGetContext_Internal(ContextSelector selector) const noexcept
	{
		if (selector.empty())
			return make_optional_cref(*this);

		auto pCtx = this;
		for (size_t i = 0; i < selector.size(); ++i)
		{
			auto& key = selector[i];
			if (auto itNext = pCtx->TryGetContext_Internal(key))
			{
				pCtx = &itNext.value();
				continue;
			}
			else
				return fig::nullref;
		}
		return make_optional_cref(*pCtx);
	}

	fig::optional_ref<Context> Context::TryGetContext_Internal(fig::handle key) noexcept
	{
		if (auto itFind = _contexts.find(key); itFind != _contexts.cend())
			return make_optional_ref((*itFind).second);
		return fig::nullref;
	}

	fig::optional_cref<Context> Context::TryGetContext_Internal(fig::handle key) const noexcept
	{
		if (auto itFind = _contexts.find(key); itFind != _contexts.cend())
			return make_optional_cref((*itFind).second);
		return fig::nullref;
	}

	void Context::Clear() noexcept
	{
		_values.clear();
		_flags.clear();
		_contexts.clear();
		_primarySelector = {};
		_pGlobalMacroProvider.reset();
		_pCustomMacroProvider = std::make_unique<fig::text::MacroProvider>();
	}

	void Context::SetAlias(fig::handle alias, const ContextLocator& target) noexcept
	{
		_pCustomMacroProvider->AddValueAlias(alias, target);
	}

	void Context::SetAlias(fig::handle alias, const ContextSelector& target) noexcept
	{
		_pCustomMacroProvider->AddSelectorAlias(alias, target);
	}

	void Context::SetMacro(fig::handle macro, const fig::string& value) noexcept
	{
		_pCustomMacroProvider->AddMacro(macro, value);
	}

	void Context::SetMacroProvider(std::weak_ptr<fig::text::MacroProvider> pMacroProvider)
	{
		_pGlobalMacroProvider = pMacroProvider;
	}
}