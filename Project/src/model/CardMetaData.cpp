#include <pch.h>
#include "model/CardMetaData.h"
#include "util/Common.h"
#include <json.hpp>

namespace fig::io
{
	static constexpr auto FlagMapping = std::array<std::tuple<CardMetaData::Flag, std::string_view>, 4> {
		std::pair { CardMetaData::Flag::New, "new" },
		std::pair { CardMetaData::Flag::Imported, "imported" },
		std::pair { CardMetaData::Flag::Hidden, "hidden" },
		std::pair { CardMetaData::Flag::Favorite, "favorite" },
	};

	static fig::string SerializeBorder(CardBorderStyle border)
	{
		switch (border) //! @temp
		{
		case CardBorderStyle::Style01: return "1";
		case CardBorderStyle::Style02: return "2";
		case CardBorderStyle::Style03: return "3";
		case CardBorderStyle::Style04: return "4";
		case CardBorderStyle::Style05: return "5";
		case CardBorderStyle::Style06: return "6";
		default: return "";
		}
	}

	static CardBorderStyle DeserializeBorder(const fig::string& border)
	{
		//! @temp
		if (border == "1") return CardBorderStyle::Style01;
		if (border == "2") return CardBorderStyle::Style02;
		if (border == "3") return CardBorderStyle::Style03;
		if (border == "4") return CardBorderStyle::Style04;
		if (border == "5") return CardBorderStyle::Style05;
		if (border == "6") return CardBorderStyle::Style06;
		return CardBorderStyle::None;
	}

	std::optional<CardMetaData> CardMetaData::FromJson(const fig::string& strJson)
	{
		if (strJson.empty())
			return std::nullopt;

		try
		{
			auto json = nlohmann::json::parse(strJson);

			CardMetaData data;

			data.borderStyle = DeserializeBorder(json.value("border_style", ""));
			
			std::vector<fig::string> f;
			json.at("flags").get_to(f); 
			data.flags.Deserialize(f, FlagMapping);
			return data;
		}
		catch (const nlohmann::json::exception&)
		{
			return std::nullopt;
		}
	}

	fig::string CardMetaData::ToJson(const CardMetaData& data)
	{
		try
		{
			nlohmann::json json;
			if (data.borderStyle != CardBorderStyle::None)
				json["border"] = SerializeBorder(data.borderStyle);
			if (!data.flags.IsEmpty())
				json["flags"] = data.flags.Serialize(FlagMapping);

			return json.dump();
		}
		catch (const nlohmann::json::exception&)
		{
			return "{}";
		}
	}
}