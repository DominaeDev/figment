#pragma once

#include "Figment.h"

namespace fig::data
{
	struct VersionNumber
	{
		uint8_t major { 0 };
		uint8_t minor { 0 };

		constexpr VersionNumber(uint8_t maj = 0, uint8_t min = 0)
			: major(maj), minor(min)
		{
		}

		constexpr explicit VersionNumber(uint16_t packed)
		{
			from_uint16(packed);
		}

		constexpr uint16_t to_uint16() const
		{
			return (static_cast<uint16_t>(major) << 8) | minor;
		}

		constexpr void from_uint16(uint16_t packed)
		{
			major = static_cast<uint8_t>((packed >> 8) & 0xFF);
			minor = static_cast<uint8_t>(packed & 0xFF);
		}

		constexpr bool operator==(const VersionNumber& other) const
		{
			return major == other.major && minor == other.minor;
		}

		constexpr bool operator!=(const VersionNumber& other) const
		{
			return !(*this == other);
		}

		constexpr bool operator<(const VersionNumber& other) const
		{
			if (major == other.major)
				return minor < other.minor;
			return major < other.major;
		}

		constexpr bool operator<=(const VersionNumber& other) const
		{
			return *this < other || *this == other;
		}

		constexpr bool operator>(const VersionNumber& other) const
		{
			return !(*this <= other);
		}

		constexpr bool operator>=(const VersionNumber& other) const
		{
			return !(*this < other);
		}

		constexpr bool is_valid() const noexcept
		{
			return major != 0 && minor != 0;
		}
	};
}