#pragma once

#include <optional>
#include <string>

template<typename T>
inline int32_t toI(T x) { return static_cast<int32_t>(x); }
template<typename T>
inline int64_t toI64(T x) { return static_cast<int64_t>(x); }
template<typename T>
inline float toF(T x) { return static_cast<float>(x); }
template<typename T>
inline double toD(T x) { return static_cast<double>(x); }

extern void DebugPrint(std::string message);
extern void DebugPrintLn(std::string message = "");

extern std::string CreateUUID();

extern std::string FormatMessage(std::string message, std::string actorName);

extern std::string& NormalizeNewlines(std::string& text);
extern std::string NormalizeNewlines(std::string&& s);
extern std::optional<std::string> ReadTextFile(const std::string& filename, bool normalizeNewlines = true);
extern bool WriteTextFile(const std::string& filename, const std::string& content, bool append = false);

