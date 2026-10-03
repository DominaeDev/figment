#pragma once

#include <stdint.h>
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

	enum class TimeFormat
	{
		HR24 = 0,
		HR12,
	};

	enum class DateFormat
	{
		YYYYMMDD = 0,
		DDMMYYYY,
		MMDDYYYY,
	};

	static auto TimeFormatMapping = std::array<std::pair<TimeFormat, std::string_view>, 2> {
		std::pair { TimeFormat::HR24,			"24hr" },
		std::pair { TimeFormat::HR12,			"12hr" }
	};

	static auto DateFormatMapping = std::array<std::pair<DateFormat, std::string_view>, 3> {
		std::pair { DateFormat::YYYYMMDD,		"ymd" },
		std::pair { DateFormat::DDMMYYYY,		"dmy" },
		std::pair { DateFormat::MMDDYYYY,		"mdy" },
	};

	struct duration
	{
		constexpr duration(const duration& other) = default;
		constexpr duration() = default;

		constexpr explicit duration(int64_t duration_ms) :
			_duration { duration_ms }
		{
		}
		constexpr explicit duration(float duration_sec) :
			_duration { static_cast<int64_t>(duration_sec * 1000.0f) }
		{
		}

		constexpr explicit operator int64_t() { return _duration; }
		operator std::chrono::milliseconds() { return std::chrono::milliseconds(_duration); }

		auto operator<=>(const duration& other) const noexcept { return _duration <=> other._duration; }
		bool operator==(const duration& other) const noexcept { return _duration == other._duration; }

		template <chrono_duration Dur>
		auto operator<=>(const Dur& duration) const noexcept { return _duration <=> std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(); }
		template <chrono_duration Dur>
		auto operator==(const Dur& duration) const noexcept { return _duration == std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(); }

		int64_t milliseconds() const { return std::chrono::milliseconds(_duration).count(); }
		int64_t seconds() const { return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(_duration)).count(); }
		int64_t minutes() const { return std::chrono::duration_cast<std::chrono::minutes>(std::chrono::milliseconds(_duration)).count(); }
		int64_t hours() const { return std::chrono::duration_cast<std::chrono::hours>(std::chrono::milliseconds(_duration)).count(); }
		int64_t days() const { return std::chrono::duration_cast<std::chrono::days>(std::chrono::milliseconds(_duration)).count(); }
		int64_t months() const { return std::chrono::duration_cast<std::chrono::months>(std::chrono::milliseconds(_duration)).count(); }
		int64_t years() const { return std::chrono::duration_cast<std::chrono::years>(std::chrono::milliseconds(_duration)).count(); }

		static duration milliseconds(int64_t count) { return duration(std::chrono::milliseconds(count).count()); }
		static duration seconds(int64_t count) { return duration(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(count)).count()); }
		static duration minutes(int64_t count) { return duration(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::minutes(count)).count()); }
		static duration hours(int64_t count) { return duration(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::hours(count)).count()); }
		static duration days(int64_t count) { return duration(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::days(count)).count()); }
		static duration months(int64_t count) { return duration(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::months(count)).count()); }
		static duration years(int64_t count) { return duration(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::years(count)).count()); }

	private:
		int64_t _duration {};
	};

	struct timestamp
	{
		timestamp() = default;
		timestamp(const timestamp&) = default;
		explicit timestamp(int64_t epoch, timezone zone = timezone::global) :
			_epoch { epoch },
			_zone { zone }
		{}

		explicit operator int64_t() { return _epoch; }
		timestamp to_local() const;
		timestamp to_global() const;
		int64_t utc_epoch() const { return static_cast<int64_t>(to_global()); }

		std::string get_time_string();
		std::string get_date_string();
		std::string get_time_string(TimeFormat clock);
		std::string get_date_string(DateFormat clock);
		std::string weekday() const;

		template <chrono_duration Dur>
		timestamp operator+(const Dur& duration) const noexcept { return timestamp(_epoch + std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()); }
		template <chrono_duration Dur>
		timestamp operator-(const Dur& duration) const noexcept { return timestamp(_epoch - std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()); }
		template <chrono_duration Dur>
		timestamp& operator+=(const Dur& duration) noexcept { _epoch += std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(); return *this; }
		template <chrono_duration Dur>
		timestamp& operator-=(const Dur& duration) noexcept { _epoch -= std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(); return *this; }
		
		timestamp operator+(duration dur) const noexcept { return timestamp(_epoch + static_cast<int64_t>(dur)); }
		timestamp operator-(duration dur) const noexcept { return timestamp(_epoch - static_cast<int64_t>(dur)); }
		timestamp& operator+=(duration dur) noexcept { _epoch += static_cast<int64_t>(dur); return *this; }
		timestamp& operator-=(duration dur) noexcept { _epoch -= static_cast<int64_t>(dur); return *this; }

		duration operator-(const timestamp& other) const noexcept { return duration(static_cast<int64_t>(utc_epoch()) - static_cast<int64_t>(other.utc_epoch())); }

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