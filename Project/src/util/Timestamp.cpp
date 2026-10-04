#include <pch.h>
#include "util/Timestamp.h"
#include "user/UserSettings.h"

namespace fig
{
	std::string timestamp::get_time_string()
	{
		if (Global::IsSignedIn())
			return get_time_string(Global::GetUserSettings().GetTimeFormat());
		return get_time_string(TimeFormat::HR24);
	}

	std::string timestamp::get_date_string()
	{
		if (Global::IsSignedIn())
			return get_date_string(Global::GetUserSettings().GetDateFormat());
		return get_date_string(DateFormat::YYYYMMDD);
	}

	std::string timestamp::get_time_string(TimeFormat format)
	{
		auto localTime = std::chrono::local_time<std::chrono::milliseconds>(*this);
		
		switch (format)
		{
		case TimeFormat::HR12:
			return trim(std::format("{:%I:%M %p}", localTime));
		default:
		case TimeFormat::HR24:
			return trim(std::format("{:%H:%M}", localTime));
		}
	}

	std::string timestamp::get_date_string(DateFormat format)
	{
		auto localTime = std::chrono::local_time<std::chrono::milliseconds>(*this);
		auto date = std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(localTime));
		auto day = static_cast<unsigned>(date.day());
		auto month = static_cast<unsigned>(date.month());
		auto year = static_cast<int32_t>(date.year());

		auto currentTime = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
		auto currentDate = std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(currentTime));
		bool includeYear = date.year() != currentDate.year();

		switch (format)
		{
		case DateFormat::DDMMYYYY:
			if (includeYear)
				return std::format("{}/{}/{}", day, month, year);
			return std::format("{0:%a}, {1} {0:%b}", localTime, day);
		case DateFormat::MMDDYYYY:
			if (includeYear)
				return std::format("%F", localTime);
			return std::format("{0:%a}, {0:%b} {1}", localTime, day);
		default:
		case DateFormat::YYYYMMDD:
			if (includeYear)
				return std::format("%D", localTime);
			return std::format("{0:%a}, {1} {0:%b}", localTime, day);
		}
	}

	std::string timestamp::weekday() const
	{
		auto localTime = std::chrono::local_time<std::chrono::milliseconds>(*this);
		return std::format("{0:%a}", localTime);
	}

	timestamp timestamp::to_local() const
	{ 
		return timestamp(std::chrono::local_time<std::chrono::milliseconds>(*this).time_since_epoch().count(), timezone::local);
	}

	timestamp timestamp::to_global() const 
	{ 
		return timestamp(std::chrono::sys_time<std::chrono::milliseconds>(*this).time_since_epoch().count(), timezone::global);
	}

}