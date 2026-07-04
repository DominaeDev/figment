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
	using MacroRef = const string_view;

	class MacroProvider
	{
	public:
		MacroProvider() = default;
		explicit MacroProvider(const fig::path& path);

		void AddMacro(fig::handle macro, const fig::string& value) noexcept;
		void AddConditionAlias(fig::handle alias, const fig::string& expression) noexcept;
		void AddSelectorAlias(fig::handle alias, const ContextSelector& target) noexcept;
		void AddValueAlias(fig::handle alias, const ContextLocator& target) noexcept;

		[[nodiscard]] std::optional<const string_view> TryGetMacro(const fig::handle& macro) const;
		[[nodiscard]] fig::optional_cref<Condition> TryGetCondition(const fig::handle& alias) const;

		bool ApplyAlias(ContextSelector& selector) const;
		bool ApplyAlias(ContextLocator& location) const;
		[[nodiscard]] std::optional<const ContextSelector> ResolveAlias(const ContextSelector& alias) const;
		[[nodiscard]] std::optional<const ContextLocator> ResolveAlias(const fig::handle& alias) const;
		[[nodiscard]] std::optional<bool> ResolveCondition(const fig::handle& alias, const Context& context) const;
		[[nodiscard]] std::optional<fig::string> ResolveMacro(const fig::handle& macro, const Context& context) const;

	private:
		std::map<fig::handle, fig::string> _macros;
		std::map<fig::handle, ContextSelector> _selectorAliases;
		std::map<fig::handle, ContextLocator> _valueAliases;
		std::map<fig::handle, Condition> _conditions;
	};
}
