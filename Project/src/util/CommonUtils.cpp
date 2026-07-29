#include <pch.h>

#include <algorithm> 
#include <cctype>
#include <locale>
#include <print>
#include <uuid_v4.h>
#include <base64.h>

namespace fig
{
	void Log(fig::string message)
	{
		if constexpr (EnableLogging)
		{
			std::print("{}", message);
		}
	}

	void LogLn(fig::string message)
	{
		if constexpr (EnableLogging)
		{
			std::println("{}", message);
		}
	}

	void MeasureTime(const fig::string& label, MeasureTimeFn fn)
	{
		auto startTime = std::chrono::steady_clock::now();
		fn();
		auto endTime = std::chrono::steady_clock::now();
		LogLn(std::format("{}: {}ms", label, toD(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count())));
	}

	fig::uuid _CreateUUID()
	{
		static UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
		return uuidGenerator.getUUID();
	}

	fig::string Base64Encode(fig::byte_span data) noexcept
	{
		return base64::encode_into<fig::string>(std::begin(data), std::end(data)).value_or("");
	}

	fig::bytes Base64Decode(fig::string_view text) noexcept
	{
		return base64::decode_into<fig::bytes>(text).value_or(fig::bytes());
	}

	std::optional<fig::bytes> TryBase64Decode(fig::string_view text) noexcept
	{
		if (auto dec = base64::decode_into<fig::bytes>(text))
			return dec.value();
		else
			return std::nullopt;
	}

}