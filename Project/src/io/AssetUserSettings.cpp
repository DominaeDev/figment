#include <pch.h>
#include "io/AssetUserSettings.h"
#include <json.hpp>

namespace fig::io
{
	static constexpr auto FlagMapping = std::array<std::pair<AssetUserSettings::Flag, std::string_view>, 3> {
		std::pair { AssetUserSettings::Flag::Imported,	"imported" },
		std::pair { AssetUserSettings::Flag::Hidden,	"hidden" },
		std::pair { AssetUserSettings::Flag::Favorite,	"favorite" },
	};

	static fig::string SerializeBorder(CardBorderStyle border)
	{
		switch (border)
		{
			case CardBorderStyle::Style01: return "1";	//! @temp
			case CardBorderStyle::Style02: return "2";	//! @temp
			case CardBorderStyle::Style03: return "3";	//! @temp
			case CardBorderStyle::Style04: return "4";	//! @temp
			case CardBorderStyle::Style05: return "5";	//! @temp
			case CardBorderStyle::Style06: return "6";	//! @temp
			default: return "";
		}
	}

	static CardBorderStyle DeserializeBorder(const fig::string& border)
	{
		if (border == "1") return CardBorderStyle::Style01;	//! @temp
		if (border == "2") return CardBorderStyle::Style02;	//! @temp
		if (border == "3") return CardBorderStyle::Style03;	//! @temp
		if (border == "4") return CardBorderStyle::Style04;	//! @temp
		if (border == "5") return CardBorderStyle::Style05;	//! @temp
		if (border == "6") return CardBorderStyle::Style06;	//! @temp
		return CardBorderStyle::None;
	}

	std::optional<AssetUserSettings> AssetUserSettings::FromJson(fig::string_view strJson)
	{
		if (strJson.empty())
			return std::nullopt;

		try
		{
			auto json = nlohmann::json::parse(strJson);

			AssetUserSettings data;

			// Border
			data.borderStyle = DeserializeBorder(json.value("border", ""));

			// Flags
			std::vector<fig::string> flags;
			json.value("flags", nlohmann::json::array()).get_to(flags);
			data.flags = Flags::Deserialize(flags, FlagMapping);

			// Order
			data.order = json.value("order", -1);
			return data;
		}
		catch (const nlohmann::json::exception&)
		{
			return std::nullopt;
		}
	}

	fig::string AssetUserSettings::ToJson(const AssetUserSettings& data)
	{
		try
		{
			nlohmann::json json;
			if (data.borderStyle != CardBorderStyle::None)
				json["border"] = SerializeBorder(data.borderStyle);
			if (!data.flags.IsEmpty())
				json["flags"] = Flags::Serialize(data.flags, FlagMapping);
			if (data.order >= 0)
				json["order"] = data.order;
			return json.dump();
		}
		catch (const nlohmann::json::exception&)
		{
			return "{}";
		}
	}
}