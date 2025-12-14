#include "util/Common.h"
#include "util/StringUtility.h"
#include "Constants.h"

#include <algorithm> 
#include <cctype>
#include <locale>
#include <uuid_v4.h>
#include <format>

namespace fig::common_util
{
	void DebugPrint(string message) noexcept
	{
#if _DEBUG
		if (message.empty())
			return;

		printf(message.c_str());
#else
		// noop
#endif
	}

	void DebugPrintLn(string message) noexcept
	{
		DebugPrint(message);
		DebugPrint("\r\n");
	}

	string CreateUUID()
	{
		static UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
		return uuidGenerator.getUUID().str();
	}
}