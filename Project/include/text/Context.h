#ifndef CONTEXTUAL_H__
#define CONTEXTUAL_H__
#pragma once

#include "Figment.h"

namespace fig
{
	struct Context;
	using ContextualValue = std::variant<int32_t, float, fig::string>;
	using ContextualRef = std::reference_wrapper<Context>;
	using ContextualCRef = std::reference_wrapper<const Context>;

	class Selector
	{
	public:
		Selector(const fig::string& source);
		Selector() = default;
		~Selector() = default;
		Selector(const Selector&) = default;
		Selector(Selector&&) = default;
		Selector& operator=(const Selector&) = default;
		Selector& operator=(Selector&&) = default;

		inline bool empty() const noexcept { return _value.empty(); }
		inline auto GetKeys() const noexcept { return std::span { _value.data(), _value.size() }; }

		Selector Append(const fig::string& key) const noexcept;
	private:
		std::vector<fig::handle> _value;
	};

	template <typename T>
	concept IContextual = requires(T t)
	{
		{ t.GetContext() } -> std::same_as<Context>;
	}
	or requires(T t)
	{
		{ t.GetContext() } -> std::same_as<const Context&>;
	};

	constexpr bool IsNumber(const ContextualValue& value)
	{
		return std::holds_alternative<int32_t>(value) or std::holds_alternative<float>(value);
	}

	struct ContextLocation
	{
		ContextLocation() = default;
		ContextLocation(const Selector& selector, const fig::handle& key = {})
		{
			this->selector = selector;
			this->key = key;
		}

		ContextLocation(const is_string_like auto& location)
		{
			fig::string s { location };
			if (size_t pos_selector = s.find(':'); pos_selector != npos)
			{
				selector = Selector { trim(s.substr(0, pos_selector)) };
				key = trim(s.substr(pos_selector + 1));
			}
			else
				key = trim(s);
		}

		inline explicit operator Selector() const noexcept
		{
			return selector.Append(key);
		}

		explicit operator fig::string() const noexcept;

		inline bool IsOk() const noexcept { return not key.empty(); }

		Selector selector;
		fig::handle key;
	};

	struct Context
	{
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
		std::optional<T> TryGetValue(ContextLocation location) const
		{
			if (location.selector.empty() and not location.key.empty())
			{
				if (auto itAlias = _valueAliases.find(location.key); itAlias != _valueAliases.cend())
					return TryGetValue_Internal<T>(itAlias->second);
			}

			return TryGetValue_Internal<T>(location);
		};

		std::optional<ContextualValue> TryGetRaw(fig::handle name) const
		{
			if (auto itAlias = _valueAliases.find(name); itAlias != _valueAliases.cend())
				return TryGetRaw_Internal(itAlias->second);
			return TryGetRaw_Internal(name);
		};

		std::optional<ContextualValue> TryGetRaw(ContextLocation location) const
		{
			if (location.selector.empty() and not location.key.empty())
			{
				if (auto itAlias = _valueAliases.find(location.key); itAlias != _valueAliases.cend())
					return TryGetRaw_Internal(itAlias->second);
			}

			return TryGetRaw_Internal(location);
		};

		bool HasValue(fig::handle flag) const noexcept
		{
			if (auto itAlias = _valueAliases.find(flag); itAlias != _valueAliases.cend())
				return HasValue_Internal(itAlias->second);
			return HasValue_Internal(flag);
		}

		bool HasValue(ContextLocation location) const
		{
			if (location.selector.empty() and not location.key.empty())
			{
				if (auto itAlias = _valueAliases.find(location.key); itAlias != _valueAliases.cend())
					return HasValue_Internal(itAlias->second);
			}

			return HasValue_Internal(location);
		};

		inline const std::map<fig::handle, ContextualValue>& GetValues() const noexcept { return _values; }
		void ClearValues() noexcept;

		template<typename T>
		inline T GetValue(fig::handle name, T default_value = {}) const
		{
			return TryGetValue<T>(name).value_or(default_value);
		};

		template<typename T>
		inline T GetValue(ContextLocation location, T default_value = {}) const
		{
			return TryGetValue<T>(location).value_or(default_value);
		};

		// Flags
		void SetFlag(fig::handle flag) noexcept;
		void SetFlags(std::span<fig::handle> flags) noexcept;
		void UnsetFlag(fig::handle flag) noexcept;
		void UnsetFlags(std::span<fig::handle> flags) noexcept;
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
		std::optional<ContextualRef> TryGetContext(fig::handle name) noexcept;
		std::optional<ContextualCRef> TryGetContext(fig::handle name) const noexcept;
		std::optional<ContextualRef> TryGetContext(Selector selector) noexcept;
		std::optional<ContextualCRef> TryGetContext(Selector selector) const noexcept;

		inline bool operator[](fig::handle name) const noexcept
		{
			return operator[]({ {}, name });
		}
		bool operator[](ContextLocation location) const noexcept;

		void Clear() noexcept;

		void AddValueAlias(fig::handle alias, const ContextLocation& target) noexcept;
		void AddSelectorAlias(fig::handle alias, const ContextLocation& target) noexcept;
		void RemoveValueAlias(fig::handle alias) noexcept;
		void RemoveSelectorAlias(fig::handle alias) noexcept;

		[[nodiscard]] const Context& GetContext() const noexcept { return *this; }

	private:
		std::optional<ContextualRef> TryGetContext(ContextLocation location) noexcept;
		std::optional<ContextualCRef> TryGetContext(ContextLocation location) const noexcept;

		template<typename T> 
		std::optional<T> TryGetValue_Internal(ContextLocation location) const noexcept
		{
			if (auto try_ctx = TryGetContext(location.selector))
			{
				auto& ctx = try_ctx.value().get();
				return ctx.TryGetValue_Internal<T>(location.key);
			}
			return std::nullopt;
		};

		bool HasValue_Internal(ContextLocation location) const noexcept
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

		std::optional<ContextualValue> TryGetRaw_Internal(ContextLocation location) const noexcept
		{
			if (auto try_ctx = TryGetContext(location.selector))
			{
				auto& ctx = try_ctx.value().get();
				return ctx.TryGetRaw_Internal(location.key);
			}
			return std::nullopt;
		};

		[[nodiscard]] std::optional<ContextualValue> TryGetRaw_Internal(fig::handle name) const noexcept;

		template<typename T> std::optional<T> TryGetValue_Internal(fig::handle name) const = delete;
		template<> [[nodiscard]] std::optional<bool> TryGetValue_Internal<bool>(fig::handle name) const noexcept;
		template<> [[nodiscard]] std::optional<int32_t> TryGetValue_Internal<int32_t>(fig::handle name) const noexcept;
		template<> [[nodiscard]] std::optional<float> TryGetValue_Internal<float>(fig::handle name) const noexcept;
		template<> [[nodiscard]] std::optional<fig::string> TryGetValue_Internal<fig::string>(fig::handle name) const noexcept;
		
		bool GetBool_Internal(fig::handle name) const noexcept;
		bool GetBool_Internal(ContextLocation location) const noexcept;

		std::unordered_set<fig::handle> _flags;
		std::map<fig::handle, ContextualValue> _values;
		std::map<fig::handle, Context> _contexts;
		std::map<fig::handle, ContextLocation> _valueAliases;
		std::map<fig::handle, ContextLocation> _selectorAliases;
	};
}

#endif
