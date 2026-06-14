#include <pch.h>
#include "text/MacroProvider.h"
#include "text/Context.h"
#include "text/TextEvaluator.h"
#include "io/Xml.h"

using namespace fig::io;

namespace fig::text
{
	MacroProvider::MacroProvider(const fig::path& path)
	{
		XmlReader xml(path, "Macros");
		if (not xml.IsOk())
			return;

		auto root = xml.GetRoot();
		
		auto macroNode = root.GetFirstElement("Macro");
		while (macroNode)
		{
			auto& node = macroNode.value();
			if (auto try_id = node["id"].TryGet<fig::handle>())
				_macros[try_id.value()] = std::move(node.GetValue<fig::string>());
			macroNode = node.GetNextSibling();
		}
		
		auto conditionNode = root.GetFirstElement("Condition");
		while (conditionNode)
		{
			auto& node = conditionNode.value();
			if (auto try_id = node["id"].TryGet<fig::handle>())
			{
				Condition condition(node.GetValue<fig::string>());
				if (condition.IsOk())
					_conditionAliases[try_id.value()] = std::move(condition);
				else
					LogLn(std::format("Failed to parse condition macro {}", try_id.value().c_str()));
			}
			conditionNode = node.GetNextSibling();
		}

		auto aliasNode = root.GetFirstElement("Alias");
		while (aliasNode)
		{
			auto& node = aliasNode.value();
			if (auto try_id = node["id"].TryGet<fig::string>())
			{
				auto value = node.GetValue<fig::string>();
				if (not value.empty())
				{
					auto& id = try_id.value();
					if (auto idx_selector = id.find(':'); idx_selector != npos)
					{
						id = id.substr(0, idx_selector);
						_selectorAliases[id] = fig::ContextSelector(value);
					}
					else
					{
						_valueAliases[id] = ContextLocator(value);
					}
				}
			}

			aliasNode = node.GetNextSibling();
		}
	}

	void MacroProvider::AddMacro(fig::handle alias, const fig::string& value) noexcept
	{
		_macros.insert_or_assign(alias, value);
	}

	void MacroProvider::AddConditionAlias(fig::handle alias, const fig::string& expression) noexcept
	{
		Condition condition(expression);
		if (condition.IsOk())
			_conditionAliases.insert_or_assign(alias, std::move(condition));
	}

	void MacroProvider::AddSelectorAlias(fig::handle alias, const ContextSelector& target) noexcept
	{
		_selectorAliases[alias] = target;
	}

	void MacroProvider::AddValueAlias(fig::handle alias, const ContextLocator& target) noexcept
	{
		_valueAliases[alias] = target;
	}

	bool MacroProvider::ApplyAlias(ContextSelector& selector) const
	{
		if (auto try_alias = ResolveAlias(selector))
		{
			selector = try_alias.value();
			return true;
		}
		return false;
	}

	bool MacroProvider::ApplyAlias(ContextLocator& location) const
	{
		if (location.selector.empty() and not location.key.empty())
		{
			if (auto try_alias = ResolveAlias(location.key))
			{
				location = try_alias.value();
				return true;
			}
		}
		else if (not location.selector.empty())
		{
			if (auto try_alias = ResolveAlias(location.selector))
			{
				location.selector = try_alias.value();
				return true;
			}
		}
		return false;
	}

	std::optional<ContextSelector> MacroProvider::ResolveAlias(const ContextSelector& selector) const
	{
		if (selector.size() == 1)
		{
			auto alias = selector[0];
			if (auto try_selector = _selectorAliases.find(alias); try_selector != _selectorAliases.cend())
				return (*try_selector).second;
		}
		return std::nullopt;
	}

	std::optional<ContextLocator> MacroProvider::ResolveAlias(const fig::handle& alias) const
	{
		if (auto try_value = _valueAliases.find(alias); try_value != _valueAliases.cend())
			return (*try_value).second;
		return std::nullopt;
	}

	std::optional<bool> MacroProvider::ResolveCondition(const fig::handle& alias, const Context& context, ContextSelector selector) const
	{
		auto itFind = _conditionAliases.find(alias);
		if (itFind == _conditionAliases.cend())
			return std::nullopt;

		if (not selector.empty())
		{
			if (auto try_ctx = context.TryGetContext(selector))
				return (*itFind).second.Evaluate(try_ctx.value().get());
			return false; // Invalid selector
		}
		return (*itFind).second.Evaluate(context);
	}

	std::optional<fig::string> MacroProvider::ResolveMacro(const fig::handle& macro, const Context& context, ContextSelector selector) const
	{
		auto itFind = _macros.find(macro);
		if (itFind == _macros.cend())
			return std::nullopt;

		if (not selector.empty())
		{
			if (auto try_ctx = context.TryGetContext(selector))
				return eval_text((*itFind).second, try_ctx.value().get());
			return ""; // Invalid selector
		}
		return eval_text((*itFind).second, context);
	}

}