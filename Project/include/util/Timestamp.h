#pragma once

#include <stdint.h>
#include <format>
#include <chrono>

namespace fig
{
	template <class _Tp>
	struct is_chrono_duration : std::false_type {};

	template <class _Rep, class _Period>
	struct is_chrono_duration<std::chrono::duration<_Rep, _Period>> : std::true_type {};

	template<typename _Tp>
	concept chrono_duration = is_chrono_duration<_Tp>::value;

	enum class timezone
	{
		global,
		local,
	};

	enum class Clock
	{
		Default,
		H12,
		H24,
	};

	static auto ClockMapping = std::array<std::pair<Clock, std::string_view>, 3> {
		std::pair { Clock::Default,	"default" },
		std::pair { Clock::H12,	"12h" },
		std::pair { Clock::H24,	"24h" }
	};

	struct timespan
	{
		timespan() = default;
		explicit timespan(int64_t duration_ms) :
			_duration { duration_ms }
		{
		}
		explicit timespan(float duration_sec) :
			_duration { static_cast<int64_t>(duration_sec * 1000.0f) }
		{
		}

		explicit operator int64_t() { return _duration; }
		operator std::chrono::milliseconds() { return std::chrono::milliseconds(_duration); }

		auto operator<=>(const timespan& other) const noexcept { return _duration <=> other._duration; }
		bool operator==(const timespan& other) const noexcept { return _duration == other._duration; }

		template <chrono_duration Dur>
		auto operator<=>(const Dur& duration) const noexcept { return _duration <=> std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(); }
		template <chrono_duration Dur>
		auto operator==(const Dur& duration) const noexcept { return _duration == std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(); }

		int64_t milliseconds() const { return std::chrono::milliseconds(_duration).count(); }
		int64_t seconds() const { return std::chrono::floor<std::chrono::seconds>(std::chrono::milliseconds(_duration)).count(); }
		int64_t minutes() const { return std::chrono::floor<std::chrono::minutes>(std::chrono::milliseconds(_duration)).count(); }
		int64_t hours() const { return std::chrono::floor<std::chrono::hours>(std::chrono::milliseconds(_duration)).count(); }
		int64_t days() const { return std::chrono::floor<std::chrono::days>(std::chrono::milliseconds(_duration)).count(); }
		int64_t months() const { return std::chrono::floor<std::chrono::months>(std::chrono::milliseconds(_duration)).count(); }
		int64_t years() const { return std::chrono::floor<std::chrono::years>(std::chrono::milliseconds(_duration)).count(); }

		static timespan milliseconds(int64_t count) { return timespan(std::chrono::milliseconds(count).count()); }
		static timespan seconds(int64_t count) { return timespan(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(count)).count()); }
		static timespan minutes(int64_t count) { return timespan(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::minutes(count)).count()); }
		static timespan hours(int64_t count) { return timespan(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::hours(count)).count()); }
		static timespan days(int64_t count) { return timespan(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::days(count)).count()); }
		static timespan months(int64_t count) { return timespan(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::months(count)).count()); }
		static timespan years(int64_t count) { return timespan(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::years(count)).count()); }

	private:
		int64_t _duration;
	};

	struct timestamp
	{
		timestamp() = default;
		explicit timestamp(int64_t epoch, timezone zone = timezone::global) :
			_epoch { epoch },
			_zone { zone }
		{}

		explicit operator int64_t() { return _epoch; }
		timestamp to_local() const;
		timestamp to_global() const;
		int64_t utc_epoch() const { return static_cast<int64_t>(to_global()); }


		std::string get_time_string(Clock clock);
		std::string get_date_string();

		template <chrono_duration Dur>
		timestamp operator+(const Dur& duration) const noexcept { return timestamp(_epoch + std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()); }
		template <chrono_duration Dur>
		timestamp operator-(const Dur& duration) const noexcept { return timestamp(_epoch - std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()); }
		template <chrono_duration Dur>
		timestamp& operator+=(const Dur& duration) noexcept { _epoch += std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(); return *this; }
		template <chrono_duration Dur>
		timestamp& operator-=(const Dur& duration) noexcept { _epoch -= std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(); return *this; }
		
		timestamp operator+(timespan dur) const noexcept { return timestamp(_epoch + static_cast<int64_t>(dur)); }
		timestamp operator-(timespan dur) const noexcept { return timestamp(_epoch - static_cast<int64_t>(dur)); }
		timestamp& operator+=(timespan dur) noexcept { _epoch += static_cast<int64_t>(dur); return *this; }
		timestamp& operator-=(timespan dur) noexcept { _epoch -= static_cast<int64_t>(dur); return *this; }

		timespan operator-(const timestamp& other) const noexcept { return timespan(static_cast<int64_t>(utc_epoch()) - static_cast<int64_t>(other.utc_epoch())); }

		auto operator<=>(const timestamp& other) const noexcept { return utc_epoch() <=> other.utc_epoch(); }
		bool operator==(const timestamp& other) const noexcept { return utc_epoch() == other.utc_epoch(); }

		template <chrono_duration Dur>
		auto operator<=>(const Dur& duration) const noexcept { return _epoch <=> std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(); }
		template <chrono_duration Dur>
		auto operator==(const Dur& duration) const noexcept { return _epoch == std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(); }

		template <chrono_duration Dur>
		explicit operator std::chrono::local_time<Dur>() const
		{
			if (_zone == timezone::global)
				return std::chrono::current_zone()->to_local(std::chrono::sys_time(std::chrono::duration_cast<Dur>(std::chrono::milliseconds(_epoch))));
			return std::chrono::local_time(std::chrono::duration_cast<Dur>(std::chrono::milliseconds(_epoch)));
		}

		template <chrono_duration Dur>
		explicit operator std::chrono::sys_time<Dur>() const
		{
			if (_zone == timezone::local)
				return std::chrono::current_zone()->to_sys(std::chrono::local_time(std::chrono::duration_cast<Dur>(std::chrono::milliseconds(_epoch))));
			return std::chrono::sys_time(std::chrono::duration_cast<Dur>(std::chrono::milliseconds(_epoch)));
		}

	private:
		int64_t _epoch {};
		timezone _zone {};
	};
}