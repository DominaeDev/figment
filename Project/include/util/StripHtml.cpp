#include <pch.h>
#include "util/StringUtils.h"
#include "util/StripHtml.h"

namespace fig
{
	static const std::unordered_map<fig::string_view, char32_t> namedEntities =
	{
		// XML / control
		{ "amp", U'&' }, { "lt", U'<' }, { "gt", U'>' }, { "quot", U'"' }, { "apos", U'\'' },

		// Latin-1 supplement
		{ "nbsp", U'\u00A0' }, { "iexcl", U'\u00A1' }, { "cent", U'\u00A2' }, { "pound", U'\u00A3' },
		{ "curren", U'\u00A4' }, { "yen", U'\u00A5' }, { "brvbar", U'\u00A6' }, { "sect", U'\u00A7' },
		{ "uml", U'\u00A8' }, { "copy", U'\u00A9' }, { "ordf", U'\u00AA' }, { "laquo", U'\u00AB' },
		{ "not", U'\u00AC' }, { "shy", U'\u00AD' }, { "reg", U'\u00AE' }, { "macr", U'\u00AF' },
		{ "deg", U'\u00B0' }, { "plusmn", U'\u00B1' }, { "sup2", U'\u00B2' }, { "sup3", U'\u00B3' },
		{ "acute", U'\u00B4' }, { "micro", U'\u00B5' }, { "para", U'\u00B6' }, { "middot", U'\u00B7' },
		{ "cedil", U'\u00B8' }, { "sup1", U'\u00B9' }, { "ordm", U'\u00BA' }, { "raquo", U'\u00BB' },
		{ "frac14", U'\u00BC' }, { "frac12", U'\u00BD' }, { "frac34", U'\u00BE' }, { "iquest", U'\u00BF' },
		{ "Agrave", U'\u00C0' }, { "Aacute", U'\u00C1' }, { "Acirc", U'\u00C2' }, { "Atilde", U'\u00C3' },
		{ "Auml", U'\u00C4' }, { "Aring", U'\u00C5' }, { "AElig", U'\u00C6' }, { "Ccedil", U'\u00C7' },
		{ "Egrave", U'\u00C8' }, { "Eacute", U'\u00C9' }, { "Ecirc", U'\u00CA' }, { "Euml", U'\u00CB' },
		{ "Igrave", U'\u00CC' }, { "Iacute", U'\u00CD' }, { "Icirc", U'\u00CE' }, { "Iuml", U'\u00CF' },
		{ "ETH", U'\u00D0' }, { "Ntilde", U'\u00D1' }, { "Ograve", U'\u00D2' }, { "Oacute", U'\u00D3' },
		{ "Ocirc", U'\u00D4' }, { "Otilde", U'\u00D5' }, { "Ouml", U'\u00D6' }, { "times", U'\u00D7' },
		{ "Oslash", U'\u00D8' }, { "Ugrave", U'\u00D9' }, { "Uacute", U'\u00DA' }, { "Ucirc", U'\u00DB' },
		{ "Uuml", U'\u00DC' }, { "Yacute", U'\u00DD' }, { "THORN", U'\u00DE' }, { "szlig", U'\u00DF' },
		{ "agrave", U'\u00E0' }, { "aacute", U'\u00E1' }, { "acirc", U'\u00E2' }, { "atilde", U'\u00E3' },
		{ "auml", U'\u00E4' }, { "aring", U'\u00E5' }, { "aelig", U'\u00E6' }, { "ccedil", U'\u00E7' },
		{ "egrave", U'\u00E8' }, { "eacute", U'\u00E9' }, { "ecirc", U'\u00EA' }, { "euml", U'\u00EB' },
		{ "igrave", U'\u00EC' }, { "iacute", U'\u00ED' }, { "icirc", U'\u00EE' }, { "iuml", U'\u00EF' },
		{ "eth", U'\u00F0' }, { "ntilde", U'\u00F1' }, { "ograve", U'\u00F2' }, { "oacute", U'\u00F3' },
		{ "ocirc", U'\u00F4' }, { "otilde", U'\u00F5' }, { "ouml", U'\u00F6' }, { "divide", U'\u00F7' },
		{ "oslash", U'\u00F8' }, { "ugrave", U'\u00F9' }, { "uacute", U'\u00FA' }, { "ucirc", U'\u00FB' },
		{ "uuml", U'\u00FC' }, { "yacute", U'\u00FD' }, { "thorn", U'\u00FE' }, { "yuml", U'\u00FF' },

		// Typography
		{ "OElig", U'\u0152' }, { "oelig", U'\u0153' }, { "Scaron", U'\u0160' }, { "scaron", U'\u0161' },
		{ "Yuml", U'\u0178' }, { "fnof", U'\u0192' }, { "circ", U'\u02C6' }, { "tilde", U'\u02DC' },
		{ "ensp", U'\u2002' }, { "emsp", U'\u2003' }, { "thinsp", U'\u2009' }, { "zwnj", U'\u200C' },
		{ "zwj", U'\u200D' }, { "lrm", U'\u200E' }, { "rlm", U'\u200F' }, { "ndash", U'\u2013' },
		{ "mdash", U'\u2014' }, { "lsquo", U'\u2018' }, { "rsquo", U'\u2019' }, { "sbquo", U'\u201A' },
		{ "ldquo", U'\u201C' }, { "rdquo", U'\u201D' }, { "bdquo", U'\u201E' }, { "dagger", U'\u2020' },
		{ "Dagger", U'\u2021' }, { "bull", U'\u2022' }, { "hellip", U'\u2026' }, { "permil", U'\u2030' },
		{ "prime", U'\u2032' }, { "Prime", U'\u2033' }, { "lsaquo", U'\u2039' }, { "rsaquo", U'\u203A' },
		{ "oline", U'\u203E' }, { "frasl", U'\u2044' }, { "euro", U'\u20AC' }, { "trade", U'\u2122' },

		// Greek
		{ "Alpha", U'\u0391' }, { "Beta", U'\u0392' }, { "Gamma", U'\u0393' }, { "Delta", U'\u0394' },
		{ "Epsilon", U'\u0395' }, { "Zeta", U'\u0396' }, { "Eta", U'\u0397' }, { "Theta", U'\u0398' },
		{ "Iota", U'\u0399' }, { "Kappa", U'\u039A' }, { "Lambda", U'\u039B' }, { "Mu", U'\u039C' },
		{ "Nu", U'\u039D' }, { "Xi", U'\u039E' }, { "Omicron", U'\u039F' }, { "Pi", U'\u03A0' },
		{ "Rho", U'\u03A1' }, { "Sigma", U'\u03A3' }, { "Tau", U'\u03A4' }, { "Upsilon", U'\u03A5' },
		{ "Phi", U'\u03A6' }, { "Chi", U'\u03A7' }, { "Psi", U'\u03A8' }, { "Omega", U'\u03A9' },
		{ "alpha", U'\u03B1' }, { "beta", U'\u03B2' }, { "gamma", U'\u03B3' }, { "delta", U'\u03B4' },
		{ "epsilon", U'\u03B5' }, { "zeta", U'\u03B6' }, { "eta", U'\u03B7' }, { "theta", U'\u03B8' },
		{ "iota", U'\u03B9' }, { "kappa", U'\u03BA' }, { "lambda", U'\u03BB' }, { "mu", U'\u03BC' },
		{ "nu", U'\u03BD' }, { "xi", U'\u03BE' }, { "omicron", U'\u03BF' }, { "pi", U'\u03C0' },
		{ "rho", U'\u03C1' }, { "sigmaf", U'\u03C2' }, { "sigma", U'\u03C3' }, { "tau", U'\u03C4' },
		{ "upsilon", U'\u03C5' }, { "phi", U'\u03C6' }, { "chi", U'\u03C7' }, { "psi", U'\u03C8' },
		{ "omega", U'\u03C9' }, { "thetasym", U'\u03D1' }, { "upsih", U'\u03D2' }, { "piv", U'\u03D6' },

		// Math and symbols
		{ "forall", U'\u2200' }, { "part", U'\u2202' }, { "exist", U'\u2203' }, { "empty", U'\u2205' },
		{ "nabla", U'\u2207' }, { "isin", U'\u2208' }, { "notin", U'\u2209' }, { "ni", U'\u220B' },
		{ "prod", U'\u220F' }, { "sum", U'\u2211' }, { "minus", U'\u2212' }, { "lowast", U'\u2217' },
		{ "radic", U'\u221A' }, { "prop", U'\u221D' }, { "infin", U'\u221E' }, { "ang", U'\u2220' },
		{ "and", U'\u2227' }, { "or", U'\u2228' }, { "cap", U'\u2229' }, { "cup", U'\u222A' },
		{ "int", U'\u222B' }, { "there4", U'\u2234' }, { "sim", U'\u223C' }, { "cong", U'\u2245' },
		{ "asymp", U'\u2248' }, { "ne", U'\u2260' }, { "equiv", U'\u2261' }, { "le", U'\u2264' },
		{ "ge", U'\u2265' }, { "sub", U'\u2282' }, { "sup", U'\u2283' }, { "nsub", U'\u2284' },
		{ "sube", U'\u2286' }, { "supe", U'\u2287' }, { "oplus", U'\u2295' }, { "otimes", U'\u2297' },
		{ "perp", U'\u22A5' }, { "sdot", U'\u22C5' },

		// Arrows
		{ "larr", U'\u2190' }, { "uarr", U'\u2191' }, { "rarr", U'\u2192' }, { "darr", U'\u2193' },
		{ "harr", U'\u2194' }, { "crarr", U'\u21B5' }, { "lArr", U'\u21D0' }, { "uArr", U'\u21D1' },
		{ "rArr", U'\u21D2' }, { "dArr", U'\u21D3' }, { "hArr", U'\u21D4' },

		// Misc symbols
		{ "spades", U'\u2660' }, { "clubs", U'\u2663' }, { "hearts", U'\u2665' }, { "diams", U'\u2666' },
		{ "loz", U'\u25CA' },
	};

	static void TrimTrailingSpaces(string& text) noexcept
	{
		while (not text.empty() and text.back() == ' ')
			text.pop_back();
	}

	static void CreateBreak(string& text, size_t count) noexcept
	{
		if (text.empty())
			return;

		TrimTrailingSpaces(text);

		size_t existing = 0;
		while (existing < text.size() and text[text.size() - 1 - existing] == '\n')
			existing += 1;

		while (existing < count)
		{
			text += '\n';
			existing += 1;
		}
	}

	static fig::string_view GetTagName(fig::string_view tag) noexcept
	{
		fig::string_view name = tag;

		if (not name.empty() and name.front() == '/')
			name.remove_prefix(1);

		size_t nameEnd = name.find_first_of(" \t\n/");
		if (nameEnd != fig::string_view::npos)
			name = name.substr(0, nameEnd);

		return name;
	}

	static void AppendParagraphBreak(fig::string& text, fig::string_view name) noexcept
	{
		if (equals(name, "br", true) or equals(name, "div", true))
			CreateBreak(text, 1);
		else if (equals(name, "p", true))
			CreateBreak(text, 2);
	}

	static bool IsNonDisplayedTag(fig::string_view name) noexcept
	{
		static constexpr fig::string_view nonDisplayedTags[] =
		{
			"script",
			"style",
			"noscript",
			"template",
		};

		for (fig::string_view candidate : nonDisplayedTags)
		{
			if (equals(name, candidate, true))
				return true;
		}

		return false;
	}

	static size_t FindClosingTag(fig::string_view html, fig::string_view tagName, size_t from) noexcept
	{
		for (size_t index = from; index + 2 < html.size(); index += 1)
		{
			if (html[index] == '<' and html[index + 1] == '/')
			{
				size_t nameStart = index + 2;
				size_t nameEnd = nameStart;

				while (nameEnd < html.size()
					and html[nameEnd] != '>'
					and html[nameEnd] != ' '
					and html[nameEnd] != '\t'
					and html[nameEnd] != '\n')
				{
					nameEnd += 1;
				}

				if (equals(html.substr(nameStart, nameEnd - nameStart), tagName, true))
				{
					size_t tagEnd = html.find('>', nameEnd);
					return tagEnd == fig::string_view::npos ? html.size() : tagEnd + 1;
				}
			}
		}

		return html.size();
	}

	static void AppendUtf8(fig::string& text, char32_t codepoint) noexcept
	{
		if (codepoint <= 0x7F)
		{
			text += static_cast<char>(codepoint);
		}
		else if (codepoint <= 0x7FF)
		{
			text += static_cast<char>(0xC0 | (codepoint >> 6));
			text += static_cast<char>(0x80 | (codepoint & 0x3F));
		}
		else if (codepoint <= 0xFFFF)
		{
			text += static_cast<char>(0xE0 | (codepoint >> 12));
			text += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
			text += static_cast<char>(0x80 | (codepoint & 0x3F));
		}
		else
		{
			text += static_cast<char>(0xF0 | (codepoint >> 18));
			text += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
			text += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
			text += static_cast<char>(0x80 | (codepoint & 0x3F));
		}
	}

	static std::optional<char32_t> DecodeEntity(fig::string_view entity) noexcept
	{
		fig::string_view body = entity.substr(1, entity.size() - 2);

		if (not body.empty() and body.front() == '#')
		{
			body.remove_prefix(1);

			int base = 10;
			if (not body.empty() and (body.front() == 'x' or body.front() == 'X'))
			{
				base = 16;
				body.remove_prefix(1);
			}

			if (body.empty())
				return std::nullopt;

			uint32_t value = 0;
			for (char digit : body)
			{
				int digitValue;
				if (digit >= '0' and digit <= '9')
					digitValue = digit - '0';
				else if (base == 16 and digit >= 'a' and digit <= 'f')
					digitValue = 10 + digit - 'a';
				else if (base == 16 and digit >= 'A' and digit <= 'F')
					digitValue = 10 + digit - 'A';
				else
					return std::nullopt;

				value = value * static_cast<uint32_t>(base) + static_cast<uint32_t>(digitValue);
			}

			return static_cast<char32_t>(value);
		}

		auto entityIterator = namedEntities.find(body);
		if (entityIterator != namedEntities.end())
			return entityIterator->second;

		return std::nullopt;
	}

	fig::string strip_html(fig::string_view html) noexcept
	{
		fig::string result;
		result.reserve(html.size());

		size_t index = 0;
		while (index < html.size())
		{
			char character = html[index];

			if (character == '<')
			{
				if (html.compare(index, 4, "<!--") == 0)
				{
					size_t commentEnd = html.find("-->", index + 4);
					index = commentEnd == fig::string_view::npos ? html.size() : commentEnd + 3;
					continue;
				}

				size_t tagEnd = html.find('>', index);
				if (tagEnd == fig::string_view::npos)
					break;

				fig::string_view tag = html.substr(index + 1, tagEnd - index - 1);
				fig::string_view name = GetTagName(tag);

				if (IsNonDisplayedTag(name))
				{
					index = FindClosingTag(html, name, tagEnd + 1);
					continue;
				}

				AppendParagraphBreak(result, name);

				index = tagEnd + 1;
			}
			else if (character == '&')
			{
				size_t entityEnd = html.find(';', index);
				if (entityEnd != fig::string_view::npos and entityEnd - index <= 12)
				{
					fig::string_view entity = html.substr(index, entityEnd - index + 1);
					std::optional<char32_t> decoded = DecodeEntity(entity);
					if (decoded)
					{
						AppendUtf8(result, *decoded);
						index = entityEnd + 1;
						continue;
					}
				}

				result += character;
				index += 1;
			}
			else if (fig::is_whitespace(character))
			{
				if (not (result.empty() or fig::is_whitespace(result.back())))
					result += ' ';
				index += 1;
			}
			else
			{
				result += character;
				index += 1;
			}
		}

		return result;
	}

	bool IsKnownHtmlTag(string_view name) noexcept
	{
		static constexpr string_view knownTags[] =
		{
			"p", "br", "div", "span", "b", "i",
			"u", "em", "strong", "ul", "ol", "li",
			"a", "img", "table", "tr", "td", "h1", "h2", "h3",
		};

		for (string_view candidate : knownTags)
		{
			if (equals(name, candidate, true))
				return true;
		}

		return false;
	}

	bool contains_html(fig::string_view text) noexcept
	{
		size_t index = 0;
		while (index < text.size())
		{
			size_t tagStart = text.find('<', index);
			if (tagStart == string_view::npos)
				break;

			size_t tagEnd = text.find('>', tagStart);
			if (tagEnd == string_view::npos)
				break;

			string_view name = GetTagName(text.substr(tagStart + 1, tagEnd - tagStart - 1));
			if (IsKnownHtmlTag(name))
				return true;

			index = tagEnd + 1;
		}

		return false;
	}
}