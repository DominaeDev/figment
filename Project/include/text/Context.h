#ifndef CONTEXTUAL_H__
#define CONTEXTUAL_H__
#pragma once

#include "Figment.h"
#include "text/ContextLocator.h"
#include "text/Condition.h"

namespace fig::text
{
	class MacroProvider;
	using MacroRef = const string_view;
}

namespace fig
{
	class Context;
	using ContextValue = std::variant<int32_t, float, fig::string>;
	using ContextRef = std::reference_wrapper<Context>;
	using ContextCRef = std::reference_wrapper<const Context>;

	template <typename T>
	concept IContextual = requires(T t)
	{
		{ t.GetContext() } -> std::same_as<Context>;
	}
	or requires(T t)
	{
		{ t.GetContext() } -> std::same_as<const Context&>;
	};

	constexpr bool IsNumber(const ContextValue& value)
	{
		return std::holds_alternative<int32_t>(value) or std::holds_alternative<float>(value);
	}

	class Context
	{
	public:
		Context();
		Context(const Context& other);
		Context(Context&& other) noexcept;
		~Context();

		Context& operator=(const Context& other);
		Context& operator=(Context&& other) noexcept;

		// Values
		template<typename T>
		void SetValue(fig::handle name, const T& value) = delete;
		
		template<> void SetValue<int32_t>(fig::handle name, const int32_t& value) noexcept;
		template<> void SetValue<float>(fig::handle name, const float& value) noexcept;
		template<> void SetValue<fig::string>(fig::handle name, const fig::string& value) noexcept;

		template<fig::is_string_like T> 
			requires (!std::same_as<T, fig::string>)
		void SetValue(fig::handle name, const T& value) noexcept
		{
			SetValue<fig::string>(name, fig::string { value });
		}

		template<typename T> 
			requires std::same_as<T, bool>
		void SetValue(fig::handle name, const T& value) noexcept
		{
			SetValue<int32_t>(name, value ? 1 : 0);
		}

		template<typename T>
		std::optional<T> TryGetValue(ContextLocator location) const
		{
			ResolveAlias(location);
			return TryGetValue_Internal<T>(location);
		};

		std::optional<ContextValue> TryGetRaw(fig::handle name) const
		{
			ContextLocator location { {}, name };
			ResolveAlias(location);
			return TryGetRaw_Internal(location);
		};

		std::optional<ContextValue> TryGetRaw(ContextLocator location) const
		{
			ResolveAlias(location);
			return TryGetRaw_Internal(location);
		};

		bool HasValue(fig::handle flag) const noexcept
		{
			ContextLocator location { {}, flag };
			ResolveAlias(location);
			return HasValue_Internal(location);
		}

		bool HasValue(ContextLocator location) const
		{
			ResolveAlias(location);
			return HasValue_Internal(location);
		};

		inline const std::map<fig::handle, ContextValue>& GetValues() const noexcept { return _values; }
		void RemoveValue(fig::handle name) noexcept;
		void ClearValues() noexcept;

		template<typename T>
		inline T GetValue(fig::handle name, T default_value = {}) const
		{
			return TryGetValue<T>(name).value_or(default_value);
		};

		template<typename T>
		inline T GetValue(ContextLocator location, T default_value = {}) const
		{
			return TryGetValue<T>(location).value_or(default_value);
		};

		// Flags
		void SetFlag(fig::handle flag) noexcept;
		void SetFlags(const std::set<fig::handle>& flags) noexcept;
		void RemoveFlag(fig::handle flag) noexcept;
		void RemoveFlags(const std::set<fig::handle>& flags) noexcept;
		void ClearFlags() noexcept;

		bool HasFlag(fig::handle flag) const noexcept;
		inline const std::unordered_set<fig::handle>& GetFlags() const noexcept { return _flags; }

		// (Sub-)contexts
		Context& AddContext(fig::handle name) noexcept;
		Context& AddContext(fig::handle name, const Context& context) noexcept;
		Context& AddContext(fig::handle name, Context&& context) noexcept;
		inline Context& AddContext(fig::handle name, IContextual auto& contextual) noexcept 
		{
			return AddContext(name, contextual.GetContext());
		}
		inline Context& AddContext(fig::handle name, const IContextual auto& contextual) noexcept
		{
			return AddContext(name, contextual.GetContext());
		}
		bool RemoveContext(fig::handle name) noexcept;
		inline std::optional<ContextRef> TryGetContext(fig::handle name) noexcept { return TryGetContext(ContextSelector { name }); }
		inline std::optional<ContextCRef> TryGetContext(fig::handle name) const noexcept { return TryGetContext(ContextSelector { name }); }
		std::optional<ContextRef> TryGetContext(ContextSelector selector) noexcept;
		std::optional<ContextCRef> TryGetContext(ContextSelector selector) const noexcept;

		inline bool operator[](fig::handle name) const noexcept
		{
			return operator[]({ {}, name });
		}
		bool operator[](ContextLocator location) const noexcept;

		void Clear() noexcept;
		[[nodiscard]] const Context& GetContext() const noexcept { return *this; }

		void SetMacroProvider(std::weak_ptr<fig::text::MacroProvider> pMacroProvider);
		std::optional<fig::text::MacroRef> TryGetMacro(const fig::handle& macro) const noexcept;
		std::optional<ConditionRef> TryGetCondition(const fig::handle& alias) const noexcept;

		void SetAlias(fig::handle alias, const ContextLocator& target) noexcept;
		void SetAlias(fig::handle alias, const ContextSelector& target) noexcept;

		void SetPrimarySelector(ContextSelector selector) { _primarySelector = selector; }
		inline std::optional<ContextRef> TryGetPrimaryContext(fig::handle name) noexcept 
		{ 
			return _primarySelector.empty() ? std::nullopt : TryGetContext(_primarySelector);
		}
	private:
		std::optional<ContextRef> TryGetContext_Internal(ContextSelector location) noexcept;
		std::optional<ContextCRef> TryGetContext_Internal(ContextSelector location) const noexcept;
		std::optional<ContextRef> TryGetContext_Internal(fig::handle key) noexcept;
		std::optional<ContextCRef> TryGetContext_Internal(fig::handle key) const noexcept;

		template<typename T> 
		std::optional<T> TryGetValue_Internal(ContextLocator location) const noexcept
		{
			if (auto try_ctx = TryGetContext(location.selector))
			{
				auto& ctx = try_ctx.value().get();
				return ctx.TryGetValue_Internal<T>(location.key);
			}
			return std::nullopt;
		};

		bool HasValue_Internal(ContextLocator location) const noexcept
		{
			if (auto try_ctx = TryGetContext(location.selector))
			{
				auto& ctx = try_ctx.value().get();
				return ctx.HasValue_Internal(location.key);
			}
			return false;
		};

		bool HasValue_Internal(fig::handle name) const noexcept
		{
			return _values.contains(name);
		}

		std::optional<ContextValue> TryGetRaw_Internal(ContextLocator location) const noexcept
		{
			if (auto try_ctx = TryGetContext(location.selector))
			{
				auto& ctx = try_ctx.value().get();
				return ctx.TryGetRaw_Internal(location.key);
			}
			return std::nullopt;
		};

		[[nodiscard]] std::optional<ContextValue> TryGetRaw_Internal(fig::handle name) const noexcept;

		template<typename T> std::optional<T> TryGetValue_Internal(fig::handle name) const = delete;
		template<> [[nodiscard]] std::optional<bool> TryGetValue_Internal<bool>(fig::handle name) const noexcept;
		template<> [[nodiscard]] std::optional<int32_t> TryGetValue_Internal<int32_t>(fig::handle name) const noexcept;
		template<> [[nodiscard]] std::optional<float> TryGetValue_Internal<float>(fig::handle name) const noexcept;
		template<> [[nodiscard]] std::optional<fig::string> TryGetValue_Internal<fig::string>(fig::handle name) const noexcept;
		
		bool GetBool_Internal(fig::handle name) const noexcept;
		bool GetBool_Internal(ContextLocator location) const noexcept;

		std::unordered_set<fig::handle> _flags;
		std::map<fig::handle, ContextValue> _values;
		std::map<fig::handle, Context> _contexts;
		ContextSelector _primarySelector;

		std::weak_ptr<fig::text::MacroProvider> _pGlobalMacroProvider {};
		std::unique_ptr<fig::text::MacroProvider> _pCustomMacroProvider {};

		void ResolveAlias(ContextSelector& selector) const noexcept;
		void ResolveAlias(ContextLocator& location) const noexcept;
	};
}

#endif
