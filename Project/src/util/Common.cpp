#include <pch.h>
#include "util/Common.h"
#include "util/StringUtility.h"

#include "Constants.h"
#include <algorithm> 
#include <cctype>
#include <locale>
#include <format>
#include <print>
#include <uuid_v4.h>
#include <base64.h>

namespace fig::common_util
{
	void Log(fig::string message)
	{
#if _DEBUG
		if (message.empty())
			return;

		std::print("{}", message);
#else
		// noop
#endif
	}

	void LogLn(fig::string message)
	{
		std::println("{}", message);
	}

	fig::string CreateUUID()
	{
		static UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
		return uuidGenerator.getUUID().str();
	}

	fig::string Base64Encode(fig::byte_span data) noexcept
	{
		return base64::encode_into<fig::string>(std::begin(data), std::end(data))
			.value_or("");
	}

	fig::bytes Base64Decode(fig::string_view text) noexcept
	{
		return base64::decode_into<fig::bytes>(text)
			.value_or(fig::bytes()); // RVO
	}

	std::optional<fig::bytes> TryBase64Decode(fig::string_view text) noexcept
	{
		if (auto dec = base64::decode_into<fig::bytes>(text))
			return dec.value();
		else
			return std::nullopt;
	}
}