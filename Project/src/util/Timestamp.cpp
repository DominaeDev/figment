#include <pch.h>
#include "util/Timestamp.h"

#if _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

namespace fig
{
	std::string timestamp::get_time_string(Clock clock)
	{
		if (clock == Clock::Default)
		{
#if _WIN32
			// Call OS to get the system locale format
			ULARGE_INTEGER largeInteger;
			largeInteger.QuadPart = (_epoch + 11644473600000ULL) * 10000ULL;

			FILETIME fileTime;
			fileTime.dwLowDateTime = largeInteger.LowPart;
			fileTime.dwHighDateTime = largeInteger.HighPart;

			SYSTEMTIME utcTime;
			FileTimeToSystemTime(&fileTime, &utcTime);

			SYSTEMTIME localTime;
			SystemTimeToTzSpecificLocalTime(nullptr, &utcTime, &localTime);

			wchar_t buffer[64];
			GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, TIME_NOSECONDS, &localTime, nullptr, buffer, 64);

			char narrowBuffer[64];
			int32_t length = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, narrowBuffer, 64, nullptr, nullptr);

			if (length > 0)
				return std::string(narrowBuffer, static_cast<size_t>(length - 1));
#endif
		}

		auto localTime = std::chrono::local_time<std::chrono::milliseconds>(*this);
		if (clock == Clock::H12)
			return trim(std::format("{:%I:%M %p}", localTime));
		else
			return trim(std::format("{:%H:%M}", localTime));
	}

	std::string timestamp::get_date_string()
	{
		auto localTime = std::chrono::local_time<std::chrono::milliseconds>(*this);
		auto day = std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(localTime)).day();

		return std::format("{0:%a}, {0:%b} {1}", localTime, static_cast<unsigned>(day));
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