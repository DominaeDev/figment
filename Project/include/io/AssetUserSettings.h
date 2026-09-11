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

	struct AssetUserSettings
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
		int32_t order { -1 };

		static std::optional<AssetUserSettings> FromJson(fig::string_view json);
		static fig::string ToJson(const AssetUserSettings& metaData);

		constexpr inline bool HasFlag(Flag flag) const
		{
			return flags.IsSet(flag);
		}
	};
}