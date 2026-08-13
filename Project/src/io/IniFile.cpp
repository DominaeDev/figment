#include <pch.h>
#include "io/IniFile.h"
#include <cerrno>
#include <charconv>
#include <fstream>

namespace fig::io
{
	static fig::string NextLine(const fig::string& content, size_t& pos)
	{
		size_t start = pos;
		size_t nl = content.find('\n', start);
		fig::string line;
		if (nl == fig::string::npos)
		{
			line = content.substr(start);
			pos = content.size();
		}
		else
		{
			line = content.substr(start, nl - start);
			pos = nl + 1;
		}

		if (!line.empty() and line.back() == '\r')
			line.resize(line.size() - 1);
		return line;
	}

	static constexpr fig::string Trim(const fig::string& s)
	{
		auto a = s.find_first_not_of(" \t");
		if (a == fig::string::npos)
			return {};
		auto b = s.find_last_not_of(" \t");
		return s.substr(a, b - a + 1);
	}

	static std::expected<fig::string, IniError> ReadMultiline(const fig::string& content, size_t& pos, const fig::string& tail)
	{
		fig::string result;
		if (!tail.empty())
		{ 
			result += tail; 
			result += '\n'; 
		}

		while (pos < content.size())
		{
			auto line = NextLine(content, pos);
			if (Trim(line) == "\"\"\"")
			{
				if (!result.empty() and result.back() == '\n')
					result.pop_back();
				return result;
			}
			result += line;
			result += '\n';
		}
		return std::unexpected(IniError::UnclosedMultilineString);
	}

	static bool IsMultiline(const fig::string& s)
	{
		return s.contains('\n');
	}

	static bool NeedsQuotes(const fig::string& s)
	{
		if (s.empty())
			return true;

		if (s.front() == ' ' or s.front() == '\t' or s.back() == ' ' or s.back() == '\t' or std::isdigit(s.front()))
			return true;

		for (char ch : s)
		{
			if (ch == '"' or ch == '\\' or ch == '[' or ch == ']' or ch == ';' or ch == '-' or ch == '#' or ch == '=' or ch == '\r')
				return true;
		}
		return false;
	}

	static fig::string Escape(const fig::string& s)
	{
		fig::string r;
		r.reserve(s.size() + 4);
		for (char c : s)
		{
			switch (c)
			{
			case '\\': r += "\\\\"; break;
			case '"':  r += "\\\""; break;
			case '\n': r += "\\n";  break;
			case '\t': r += "\\t";  break;
			case '\r': r += "\\r";  break;
			default:   r += c;      break;
			}
		}
		return r;
	}

	static fig::string Unescape(const fig::string& s)
	{
		fig::string r;
		r.reserve(s.size());
		for (size_t i = 0; i < s.size(); ++i)
		{
			if (s[i] != '\\' or i + 1 >= s.size())
			{ 
				r += s[i]; 
				continue; 
			}
			++i;
			switch (s[i])
			{
			case '\\': r += '\\'; break;
			case '"':  r += '"';  break;
			case 'n':  r += '\n'; break;
			case 't':  r += '\t'; break;
			case 'r':  r += '\r'; break;
			default:   r += '\\'; r += s[i]; break;
			}
		}
		return r;
	}

	static fig::string SerializeValue(const IniFile::Value& val)
	{
		return std::visit([]<typename T>(const T & value) -> fig::string
		{
			if constexpr (std::is_same_v<T, int32_t>)
			{
				return int_to_string(value);
			}
			else if constexpr (std::is_same_v<T, fig::fixed>)
			{
				return fixed_to_string(value);
			}
			else if constexpr (std::is_same_v<T, fig::string>)
			{
				if (IsMultiline(value))
					return "\"\"\"\n" + value + "\n\"\"\"";
				if (NeedsQuotes(value))
					return '"' + Escape(value) + '"';
				return value;
			}
			else if constexpr (std::is_same_v<T, std::vector<int32_t>>)
			{
				fig::string r = "[";
				for (size_t i = 0; i < value.size(); ++i)
				{
					if (i) r += ", ";
					r += int_to_string(value[i]);
				}
				return r + "]";
			}
			else if constexpr (std::is_same_v<T, std::vector<fig::fixed>>)
			{
				fig::string r = "[";
				for (size_t i = 0; i < value.size(); ++i)
				{
					if (i) r += ", ";
					r += fixed_to_string(value[i]);
				}
				return r + "]";
			}
			else /* std::vector<fig::string> */
			{
				fig::string r = "[";
				for (size_t i = 0; i < value.size(); ++i)
				{
					if (i) r += ", ";
					r += '"';
					r += Escape(value[i]);
					r += '"';
				}
				return r + "]";
			}
		}, val);
	}

	static std::vector<fig::string> SplitList(const fig::string& s)
	{
		std::vector<fig::string> items;
		size_t start = 0;
		bool in_quote = false;
		for (size_t i = 0; i < s.size(); ++i)
		{
			if (s[i] == '"' and (i == 0 or s[i - 1] != '\\'))
				in_quote = !in_quote;
			if (!in_quote and s[i] == ',')
			{
				items.push_back(Trim(s.substr(start, i - start)));
				start = i + 1;
			}
		}

		auto tail = Trim(s.substr(start));
		if (!tail.empty()) 
			items.push_back(tail);
		return items;
	}

	static IniFile::Value ParseList(const fig::string& strList)
	{
		fig::string str = strList;

		if (str.size() >= 2 and str.front() == '[' and str.back() == ']')
			str = Trim(str.substr(1, str.size() - 2));
		if (str.empty()) 
			return std::vector<fig::string>{};

		auto items = SplitList(str);
		if (items.empty()) 
			return std::vector<fig::string>{};

		// If every item is quoted, it's a string list.
		bool all_quoted = std::ranges::all_of(items, [](const fig::string&  i) {
			return i.size() >= 2 and i.front() == '"' and i.back() == '"';
		});
		if (all_quoted)
		{
			std::vector<fig::string> result;
			result.reserve(items.size());
			for (auto& item : items)
				result.push_back(Unescape(item.substr(1, item.size() - 2)));
			return result;
		}

		// Try fixed list.
		{
			std::vector<fig::fixed> result;
			result.reserve(items.size());
			bool ok = true;
			for (auto& item : items)
			{
				if (auto try_fixed = string_to_fixed(item))
					result.push_back(try_fixed.value());
				else
				{ 
					ok = false; 
					break;
				}
			}
			if (ok) 
				return result;
		}

		// Fall back to unquoted-or-mixed string list
		std::vector<fig::string> result;
		result.reserve(items.size());
		for (auto& item : items)
		{
			if (item.size() >= 2 and item.front() == '"' and item.back() == '"')
				result.push_back(Unescape(item.substr(1, item.size() - 2)));
			else
				result.push_back(item);
		}
		return result;
	}

	static IniFile::Value ParseValue(const fig::string& str)
	{
		if (str.empty())
			return fig::string {};

		// List
		if (str.front() == '[' and str.back() == ']')
			return ParseList(str);

		// Quoted string
		if (str.size() >= 2 and str.front() == '"' and str.back() == '"')
			return Unescape(str.substr(1, str.size() - 2));

		// Try fixed
		{
			fig::fixed value {};
			if (auto try_fixed = string_to_fixed(str))
				return try_fixed.value();
		}

		// Plain unquoted string
		return str;
	}

	static IniError ErrnoToReadError()
	{
		switch (errno)
		{
		case ENOENT: return IniError::FileNotFound;
		case EACCES: return IniError::FileAccessDenied;
		default:     return IniError::FileReadError;
		}
	}

	static IniError ErrnoToWriteError()
	{
		switch (errno)
		{
		case EACCES: return IniError::FileAccessDenied;
		default:     return IniError::FileWriteError;
		}
	}

	void IniFile::Set(const fig::string_view section, const fig::string_view key, fig::fixed value)
	{
		SetValue(section, key, value);
	}

	void IniFile::Set(const fig::string_view section, const fig::string_view key, fig::string value)
	{
		SetValue(section, key, std::move(value));
	}

	void IniFile::Set(const fig::string_view section, const fig::string_view key, std::vector<fig::fixed> value)
	{
		SetValue(section, key, std::move(value));
	}

	void IniFile::Set(const fig::string_view section, const fig::string_view key, std::vector<fig::string> value)
	{
		SetValue(section, key, std::move(value));
	}

	[[nodiscard]] bool IniFile::HasKey(const fig::string_view section, fig::string_view key) const
	{
		if (auto itSection = _sections.find(section); itSection != _sections.end())
			return itSection->second.values.contains(key);
		return false;
	}

	[[nodiscard]] bool IniFile::HasSection(const fig::string_view section) const
	{
		return _sections.contains(section);
	}

	void IniFile::Remove(fig::string_view section, fig::string_view key)
	{
		auto itSection = _sections.find(section);
		if (itSection == _sections.end())
			return;

		itSection->second.values.erase(key);
		std::erase(itSection->second.key_order, key);
	}

	void IniFile::RemoveSection(fig::string_view section)
	{
		_sections.erase(section);
		std::erase(_section_order, section);
	}

	void IniFile::Clear()
	{
		_sections.clear();
		_section_order.clear();
	}

	[[nodiscard]] fig::string IniFile::Serialize() const
	{
		fig::string out;
		for (const auto& section : _section_order)
		{
			auto itSection = _sections.find(section);
			if (itSection == _sections.end()) 
				continue;

			out += '[';
			out += section;
			out += "]\n";

			for (const auto& key : itSection->second.key_order)
			{
				auto itValue = itSection->second.values.find(key);
				if (itValue == itSection->second.values.end()) 
					continue;
				out += key;
				out += " = ";
				out += SerializeValue(itValue->second);
				out += '\n';
			}
			out += '\n';
		}
		return out;
	}

	[[nodiscard]] std::expected<void, IniError> IniFile::Deserialize(const fig::string& content)
	{
		Clear();

		fig::string current_group;
		bool in_group = false;
		size_t pos = 0;

		while (pos < content.size())
		{
			auto trimmed = Trim(NextLine(content, pos));

			if (trimmed.empty() or trimmed[0] == ';' or trimmed[0] == '#')
				continue;

			// [section]
			if (trimmed[0] == '[')
			{
				auto close = trimmed.find(']', 1);
				if (close == fig::string::npos)
					return std::unexpected(IniError::MalformedSection);

				current_group = Trim(trimmed.substr(1, close - 1));
				in_group = true;

				if (!_sections.contains(current_group))
				{
					_section_order.push_back(current_group);
					_sections[current_group] = {};
				}
				continue;
			}

			// key = value
			auto eq = trimmed.find('=');
			if (eq == fig::string::npos)
				continue;

			if (!in_group)
				return std::unexpected(IniError::KeyBeforeSection);

			auto key_sv = Trim(trimmed.substr(0, eq));
			if (key_sv.empty())
				return std::unexpected(IniError::EmptyKeyName);

			fig::string key(key_sv);
			auto val_sv = Trim(trimmed.substr(eq + 1));

			Value parsed;
			if (val_sv.starts_with(R"(""")"))
			{
				auto result = ReadMultiline(content, pos, val_sv.substr(3));
				if (!result)
					return std::unexpected(result.error());
				parsed = std::move(*result);
			}
			else
			{
				parsed = ParseValue(val_sv);
			}

			auto& grp = _sections[current_group];
			if (!grp.values.contains(key))
				grp.key_order.push_back(key);
			grp.values[key] = std::move(parsed);
		}
		return {};
	}

	[[nodiscard]] std::expected<void, IniError> IniFile::Save(const fig::path& path) const
	{
		std::ofstream f(path, std::ios::binary);
		if (!f)
			return std::unexpected(ErrnoToWriteError());

		auto s = Serialize();
		f.write(s.data(), static_cast<std::streamsize>(s.size()));
		f.flush();

		if (!f.good())
			return std::unexpected(IniError::FileWriteError);

		return {};
	}

	[[nodiscard]] std::expected<void, IniError> IniFile::Load(const fig::path& path)
	{
		std::ifstream f(path, std::ios::binary | std::ios::ate);
		if (!f)
			return std::unexpected(ErrnoToReadError());

		auto size = static_cast<std::streamsize>(f.tellg());
		f.seekg(0);
		fig::string content(static_cast<size_t>(size), '\0');
		f.read(content.data(), size);

		if (!f.good())
			return std::unexpected(IniError::FileReadError);

		return Deserialize(content);
	}

	void IniFile::SetValue(fig::string_view section, fig::string_view key, Value value)
	{
		fig::string str_section { section };
		fig::string str_key { key };

		if (!_sections.contains(section))
		{
			_section_order.push_back(str_section);
			_sections.emplace(str_section, Section {});
		}

		auto& newSection = _sections[str_section];
		if (!newSection.values.contains(str_key))
			newSection.key_order.push_back(str_key);
		newSection.values[str_key] = std::move(value);
	}

	
}