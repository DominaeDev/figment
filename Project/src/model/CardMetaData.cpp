#include <pch.h>
#include "model/CardMetaData.h"
#include "util/Common.h"
#include <json.hpp>

namespace fig::io
{
	static constexpr auto FlagMapping = std::array<std::pair<CardMetaData::Flag, std::string_view>, 3> {
		std::pair { CardMetaData::Flag::Imported,	"imported" },
		std::pair { CardMetaData::Flag::Hidden,		"hidden" },
		std::pair { CardMetaData::Flag::Favorite,	"favorite" },
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

	std::optional<CardMetaData> CardMetaData::FromJson(const fig::string& strJson)
	{
		if (strJson.empty())
			return std::nullopt;

		try
		{
			auto json = nlohmann::json::parse(strJson);

			CardMetaData data;

			data.borderStyle = DeserializeBorder(json.value("border", ""));
			
			std::vector<fig::string> f;
			json.at("flags").get_to(f); 
			data.flags = Flags::Deserialize(f, FlagMapping);
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
				json["flags"] = Flags::Serialize(data.flags, FlagMapping);

			return json.dump();
		}
		catch (const nlohmann::json::exception&)
		{
			return "{}";
		}
	}

	bool CardMetaData::IsNew() const noexcept
	{
		if (createdAt != updatedAt or createdAt != lastUsedAt)
			return false;
		
		fig::timestamp now = fig::util::utc_now();
		if (static_cast<long long>(now - createdAt) > std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::days(3)).count())
			return false;
		return true;
	}
}