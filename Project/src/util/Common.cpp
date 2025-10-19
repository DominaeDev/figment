#include "util/Common.h"
#include "util/StringUtility.h"
#include "Constants.h"

#include <algorithm> 
#include <cctype>
#include <locale>
#include <fstream>
#include <uuid_v4.h>
#include <format>
#include <filesystem>

std::optional<std::string> ReadTextFile(const std::string& filename, bool normalizeNewlines)
{
	try
	{
		std::ifstream file(filename.c_str(), std::ios::binary | std::ios::in | std::ios::ate);
		if (!file)
			return std::nullopt;

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::string content;
		content.reserve(size);
		content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		if (!content.empty())
			return normalizeNewlines ? string_util::normalize_newlines(content) : content;
		return std::nullopt; // Empty file
	}
	catch (...)
	{
		return std::nullopt;
	}
}

bool ReadTextFile(const std::string& filename, std::string& out_content, bool normalizeNewlines)
{
	auto content = ReadTextFile(filename, normalizeNewlines);
	if (content.has_value())
	{
		out_content = content.value();
		return true;
	}
	return false;
}

bool WriteTextFile(const std::string& filename, const std::string& content, bool append)
{
	try
	{
		std::ofstream file(filename.c_str(), std::ios::binary | std::ios::out | (append ? std::ios::app : std::ios::trunc));
		if (!file.is_open())
			return false;

		file.write(content.c_str(), content.length());
		return !file.fail();
	}
	catch (...)
	{
		return false;
	}
}

void DebugPrint(std::string message)
{
#if _DEBUG
	if (message.empty())
		return;

	printf(message.c_str());
#else
	// noop
#endif
}

void DebugPrintLn(std::string message)
{
	DebugPrint(message);
	DebugPrint("\r\n");
}

std::string CreateUUID()
{
	static UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
	return uuidGenerator.getUUID().str();
}

std::vector<std::string> FindFilesInPath(const std::string& dirPath, const std::string& extension)
{
	std::vector<std::string> matchingFiles;
	std::filesystem::path directory(dirPath);

	if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
		return matchingFiles;

	for (const auto& entry : std::filesystem::directory_iterator(directory))
	{
		if (std::filesystem::is_regular_file(entry) && (extension.empty() || string_util::equals(entry.path().extension().string(), extension, true)))
			matchingFiles.push_back(entry.path().string());
	}

	return matchingFiles;
}