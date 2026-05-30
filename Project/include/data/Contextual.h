#ifndef CONTEXTUAL_H__
#define CONTEXTUAL_H__
#pragma once

#include "Figment.h"

namespace fig::data
{
	class Contextual;
	using ContextualValue = std::variant<int32_t, float, fig::string>;
	using ContextualRef = std::reference_wrapper<Contextual>;

	class Contextual
	{
	public:
		// Values
		template<typename T>
		void SetValue(fig::handle name, T value) = delete;
		template<> void SetValue<int32_t>(fig::handle name, int32_t value) noexcept;
		template<> void SetValue<float>(fig::handle name, float value) noexcept;
		template<> void SetValue<fig::string>(fig::handle name, fig::string value) noexcept;

		template<typename T> std::optional<T> TryGetValue(fig::handle name) const = delete;
		template<> [[nodiscard]] std::optional<int32_t> TryGetValue<int32_t>(fig::handle name) const noexcept;
		template<> [[nodiscard]] std::optional<float> TryGetValue<float>(fig::handle name) const noexcept;
		template<> [[nodiscard]] std::optional<fig::string> TryGetValue<fig::string>(fig::handle name) const noexcept;

		bool HasValue(fig::handle flag) const noexcept;
		inline const std::map<fig::handle, ContextualValue>& GetValues() const noexcept { return _values; }
		void ClearValues() noexcept;

		// Flags
		void SetFlag(fig::handle flag) noexcept;
		void SetFlags(std::span<fig::handle> flags) noexcept;
		void UnsetFlag(fig::handle flag) noexcept;
		void UnsetFlags(std::span<fig::handle> flags) noexcept;
		void ClearFlags() noexcept;

		bool HasFlag(fig::handle flag) const noexcept;
		inline const std::unordered_set<fig::handle>& GetFlags() const noexcept { return _flags; }

		// (Sub-)contexts
		void AddContext(fig::handle name, Contextual& other) noexcept;
		bool RemoveContext(fig::handle name) noexcept;
		std::optional<ContextualRef> GetContext(fig::handle name) noexcept;

		bool operator[](fig::handle name) const noexcept;

	private:
		std::map<fig::handle, ContextualValue> _values;
		std::unordered_set<fig::handle> _flags;
		std::map<fig::handle, ContextualRef> _contexts;
	};	
}

#endif
