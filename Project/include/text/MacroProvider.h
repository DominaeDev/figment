#ifndef MACRO_PROVIDER_H__
#define MACRO_PROVIDER_H__
#pragma once

#include "Figment.h"
#include "text/Condition.h"
#include "text/ContextLocator.h"
	
namespace fig
{
	class Context;
}

namespace fig::text
{
	class MacroProvider
	{
	public:
		MacroProvider() = default;
		explicit MacroProvider(const fig::path& path);

		void AddMacro(fig::handle alias, const fig::string& value) noexcept;
		void AddConditionAlias(fig::handle alias, const fig::string& expression) noexcept;
		void AddSelectorAlias(fig::handle alias, const ContextSelector& target) noexcept;
		void AddValueAlias(fig::handle alias, const ContextLocator& target) noexcept;

		bool ApplyAlias(ContextSelector& selector) const;
		bool ApplyAlias(ContextLocator& location) const;
		[[nodiscard]] std::optional<ContextSelector> ResolveAlias(const ContextSelector& alias) const;
		[[nodiscard]] std::optional<ContextLocator> ResolveAlias(const fig::handle& alias) const;
		[[nodiscard]] std::optional<bool> ResolveCondition(const fig::handle& alias, const Context& context, ContextSelector selector = {}) const;
		[[nodiscard]] std::optional<fig::string> ResolveMacro(const fig::handle& macro, const Context& context, ContextSelector selector = {}) const;

	private:
		std::map<fig::handle, fig::string> _macros;
		std::map<fig::handle, ContextSelector> _selectorAliases;
		std::map<fig::handle, ContextLocator> _valueAliases;
		std::map<fig::handle, Condition> _conditionAliases;
	};
}

#endif