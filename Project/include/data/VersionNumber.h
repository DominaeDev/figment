#pragma once

#include "Figment.h"

namespace fig::data
{
	struct VersionNumber
	{
		uint8_t major { 0 };
		uint8_t minor { 0 };
		uint8_t build { 0 };

		constexpr VersionNumber(uint8_t major = 0, uint8_t minor = 0, uint8_t build = 0)
		{
			this->major = major;
			this->minor = minor;
			this->build = build;
		}

		constexpr explicit VersionNumber(uint32_t packed)
		{
			major = static_cast<uint8_t>((packed >> 16) & 0xFF);
			minor = static_cast<uint8_t>((packed >> 8) & 0xFF);
			build = static_cast<uint8_t>(packed & 0xFF);
		}

		operator std::string() const
		{
			return std::format("{}.{}.{}", major, minor, build);
		}

		explicit VersionNumber(std::string_view str)
		{
			std::array<uint8_t, 3> fields {};

			for (size_t i = 0; i < fields.size(); ++i)
			{
				if (i > 0)
				{
					if (str.empty() or str.front() != '.')
						break;
					str.remove_prefix(1);
				}

				const char* first = str.data();
				const char* last = first + str.size();
				const auto [ptr, ec] = std::from_chars(first, last, fields.at(i));
				if (ec != std::errc {})
					break;
				
				str.remove_prefix(static_cast<size_t>(ptr - first));
			}

			if (str.empty())
			{
				major = fields.at(0);
				minor = fields.at(1);
				build = fields.at(2);
			}
		}

		auto operator<=>(const VersionNumber&) const = default;
		bool operator==(const VersionNumber&) const = default;

		constexpr bool is_valid() const noexcept
		{
			return major != 0 || minor != 0 || build != 0;
		}
	};
}