#pragma once

#include <optional>
#include <string>

// Debugging
extern void DebugPrint(std::string message);
extern void DebugPrintLn(std::string message = "");

// File IO
extern std::optional<std::string> ReadTextFile(const std::string& filename, bool normalizeNewlines = true);
extern bool WriteTextFile(const std::string& filename, const std::string& content, bool append = false);

extern std::string CreateUUID();
