#pragma once

namespace fig::io
{
	enum CardBorderStyle
	{
		None,
		Style01,
		Style02,
		Style03,
		Style04,
		Style05,
		Style06,
	};

	struct ContentUserSettings
	{
		enum class Flag
		{
			Imported = 1 << 0,
			Hidden = 1 << 1,
			Favorite = 1 << 2,
		};
		using Flags = EnumFlags<Flag>;

		CardBorderStyle borderStyle {};
		Flags flags {};

		static std::optional<ContentUserSettings> FromJson(const fig::string& json);
		static fig::string ToJson(const ContentUserSettings& metaData);

		constexpr inline bool HasFlag(Flag flag) const
		{
			return flags.IsSet(flag);
		}
	};
}