#include "util/Common.h"
#include "util/StringUtility.h"
#include "Constants.h"

#include <algorithm> 
#include <cctype>
#include <locale>
#include <uuid_v4.h>
#include <format>

void common_util::DebugPrint(fig::string message) noexcept
{
#if _DEBUG
	if (message.empty())
		return;

	printf(message.c_str());
#else
	// noop
#endif
}

void common_util::DebugPrintLn(fig::string message) noexcept
{
	DebugPrint(message);
	DebugPrint("\r\n");
}

fig::string common_util::CreateUUID()
{
	static UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
	return uuidGenerator.getUUID().str();
}
