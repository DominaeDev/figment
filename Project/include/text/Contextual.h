#ifndef CONTEXTUAL_H__
#define CONTEXTUAL_H__
#pragma once

#include "Figment.h"

namespace fig
{
	struct Contextual;
	using ContextualValue = std::variant<int32_t, float, fig::string>;
	using ContextualRef = std::reference_wrapper<Contextual>;
	using ContextualCRef = std::reference_wrapper<const Contextual>;

	class Selector
	{
	public:
		Selector(const fig::string& input);
		Selector() = default;
		~Selector() = default;
		Selector(const Selector&) = default;
		Selector(Selector&&) = default;
		Selector& operator=(const Selector&) = default;
		Selector& operator=(Selector&&) = default;

		inline bool empty() const noexcept { return _value.empty(); }
		inline auto GetKeys() const noexcept { return std::span { _value.data(), _value.size() }; }
	private:
		std::vector<fig::handle> _value;
	};

	struct Contextual
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

		template<typename T> std::optional<T> TryGetValue(fig::handle name) const = delete;
		template<> [[nodiscard]] std::optional<bool> TryGetValue<bool>(fig::handle name) const noexcept;
		template<> [[nodiscard]] std::optional<int32_t> TryGetValue<int32_t>(fig::handle name) const noexcept;
		template<> [[nodiscard]] std::optional<float> TryGetValue<float>(fig::handle name) const noexcept;
		template<> [[nodiscard]] std::optional<fig::string> TryGetValue<fig::string>(fig::handle name) const noexcept;

		bool HasValue(fig::handle flag) const noexcept;
		inline const std::map<fig::handle, ContextualValue>& GetValues() const noexcept { return _values; }
		void ClearValues() noexcept;

		template<typename T> 
		inline T GetValue(fig::handle name, T default_value = {}) const
		{
			return TryGetValue<T>(name).value_or(default_value);
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
		Contextual& AddContext(fig::handle name) noexcept;
		bool RemoveContext(fig::handle name) noexcept;
		std::optional<ContextualRef> TryGetContext(fig::handle name) noexcept;
		std::optional<ContextualCRef> TryGetContext(fig::handle name) const noexcept;
		std::optional<ContextualRef> TryGetContext(Selector selector) noexcept;
		std::optional<ContextualCRef> TryGetContext(Selector selector) const noexcept;
		Contextual GetContext() const noexcept;

		bool operator[](fig::handle name) const noexcept;

	private:
		std::map<fig::handle, ContextualValue> _values;
		std::unordered_set<fig::handle> _flags;
		std::map<fig::handle, Contextual> _contexts;
	};

	template <typename T>
	concept IContextual = requires(T t)
	{
		{ t.GetContext() } -> std::same_as<Contextual>;
	};
}

#endif
