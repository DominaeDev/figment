#ifndef COMMON_UTILITY_H__
#define COMMON_UTILITY_H__

#include "Types.h"

#include <optional>
#include <vector>
#include <memory>
#include <iterator>
#include <algorithm>
#include <functional>
#include <chrono>

namespace fig::util
{
#if _DEBUG || _CONSOLE
	constexpr bool EnableLogging = true;
#else
	constexpr bool EnableLogging = false;
#endif

	// Debugging
	void Log(fig::string message);
	void LogLn(fig::string message = "");

	template<typename... Args>
	void Log(string fmt, Args... args)
	{
		if constexpr (EnableLogging)
		{
			Log(std::format(fmt, args));
		}
	}

	template<typename... Args>
	void LogLn(string fmt, Args... args)
	{
		if constexpr (EnableLogging)
		{
			LogLn(std::format(fmt, args));
		}
	}

	using MeasureTimeFn = std::function<void()>;
	void MeasureTime(const fig::string& label, MeasureTimeFn fn);

	fig::uuid CreateUUID();
	inline fig::string CreateStrUUID()
	{
		return CreateUUID().to_str();
	}

	fig::string Base64Encode(fig::byte_span data) noexcept;
	fig::bytes Base64Decode(fig::string_view) noexcept;

	inline fig::timestamp utc_now() noexcept
	{
		return static_cast<fig::timestamp>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
	}

	inline fig::timestamp local_now() noexcept
	{
		return static_cast<fig::timestamp>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::current_zone()->to_local(std::chrono::system_clock::now()).time_since_epoch()).count());
	}

	template<template <typename, typename> class Cont, typename V, typename A = std::allocator<V>>
	void container_prepend(Cont<V, A>& contA, const Cont<V, A>& contB) noexcept
	{
		contA.insert(std::begin(contA), std::begin(contB), std::end(contB));
	}

	template<template <typename, typename> class Cont, typename V, typename A = std::allocator<V>>
	void container_append(Cont<V, A>& contA, const Cont<V, A>& contB) noexcept
	{
		contA.insert(std::end(contA), std::begin(contB), std::end(contB));
	}

	template<template <typename, typename> class Cont, typename V, typename A = std::allocator<V>>
	bool container_remove_at(Cont<V, A>& cont, size_t index) noexcept
	{
		auto it = std::cbegin(cont);
		std::advance(it, index);
		if (it < std::cend(cont))
		{
			cont.erase(it);
			return false;
		}
		return false;
	}

	template<template <typename, typename> class Cont, typename V, typename A = std::allocator<V>>
	Cont<V, A> container_concat(const Cont<V, A>& contA, const Cont<V, A>& contB) noexcept
	{
		Cont<V, A> result;
		result.reserve(contA.size() + contB.size());
		result.insert(std::end(result), std::begin(contA), std::end(contA));
		result.insert(std::end(result), std::begin(contB), std::end(contB));
		return result;
	}

	template <typename T, typename A = std::allocator<T>>
	constexpr std::vector<T, A>::iterator flip_iterator(typename std::vector<T, A>& vec, typename std::vector<T, A>::reverse_iterator rit)
	{
		if (rit != vec.rend())
		{
			auto it = vec.begin();
			std::advance(it, (ptrdiff_t)std::distance(rit, vec.rend()) - 1);
			return it;
		}
		return vec.end();
	}

	template <typename T, typename A = std::allocator<T>>
	constexpr std::vector<T, A>::const_iterator flip_iterator(typename const std::vector<T, A>& vec, typename std::vector<T, A>::const_reverse_iterator rit)
	{
		if (rit != vec.crend())
		{
			auto it = vec.cbegin();
			std::advance(it, (ptrdiff_t)std::distance(rit, vec.crend()) - 1);
			return it;
		}
		return vec.cend();
	}

	template <std::floating_point T>
	inline constexpr int32_t ceil_int(T f)
	{
		return static_cast<int32_t>(std::ceilf(f));
	}

	template <std::floating_point T>
	inline constexpr int32_t floor_int(T f)
	{
		return static_cast<int32_t>(std::floorf(f));
	}

	template<template <typename, typename> class Cont, typename V, typename Pred, typename A = std::allocator<V>>
	inline constexpr Cont<V, A>::const_iterator find_last_if(const Cont<V, A>& cont, Pred pred)
	{
		auto itFind = std::find_if(cont.rbegin(), cont.rend(), pred);
		return flip_iterator<V>(cont, itFind);
	};

	template<template <typename, typename> class Cont, typename V, typename Pred, typename A = std::allocator<V>>
	inline Cont<V, A>::iterator find_last_if(Cont<V, A>& cont, Pred pred)
	{
		auto itFind = std::find_if(cont.rbegin(), cont.rend(), pred);
		return flip_iterator<V>(cont, itFind);
	};

	template<template <typename, typename> class Cont, typename V, typename Pred, typename A = std::allocator<V>>
	inline constexpr size_t find_index(const Cont<V, A>& cont, Pred pred)
	{
		auto it = std::find_if(cont.cbegin(), cont.cend(), pred);
		if (it != cont.cend())
			return toI(std::distance(cont.cbegin(), it));
		return fig::npos;
	};

	template<template <typename, typename> class Cont, typename V, typename Pred, typename A = std::allocator<V>>
	inline constexpr size_t index_of(const Cont<V, A>& cont, V value)
	{
		auto it = std::find(cont.cbegin(), cont.cend(), value);
		if (it != cont.cend())
			return toI(std::distance(cont.cbegin(), it));
		return fig::npos;
	};

	inline constexpr int32_t roundToInt(float value)
	{
		return toI(std::roundf(value));
	}

	template <typename T>
	bool is_zero(std::span<T> data) noexcept
	{
		for (T& value : data)
		{
			if (value != (T)0)
				return false;
		}
		return true;
	}

	template <typename T, size_t N>
	bool is_zero(std::array<T, N> arr) noexcept
	{
		for (auto it = std::begin(arr); it != std::end(arr); ++it)
		{
			if (*it != (T)0)
				return false;
		}
		return true;
	}

	inline bool flt_eq(float a, float b) noexcept
	{
		return std::fabs(a - b) <= std::numeric_limits<float>::epsilon() * std::max(std::fabs(a), std::fabs(b));
	}

	inline bool dbl_eq(double a, double b) noexcept
	{
		return std::abs(a - b) <= std::numeric_limits<double>::epsilon() * std::max(std::abs(a), std::abs(b));
	}

	template <typename K, typename T>
	inline std::optional<K> find_key(const std::map<K, T>& map, const T& value)
	{
		for (auto& kvp : map)
			if (kvp.second == value)
				return kvp.first;
		return std::nullopt;
	}

#if _DEBUG || _CONSOLE
	#define DEBUG_MEASURE_BEGIN(LABEL) fig::util::MeasureTime((LABEL), [&](){
	#define DEBUG_MEASURE_END() });
#else
	#define DEBUG_MEASURE_BEGIN(LABEL)
	#define DEBUG_MEASURE_END()
#endif

}
#endif