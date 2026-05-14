#ifndef CARD_META_DATA_H__
#define CARD_META_DATA_H__
#pragma once

#include "Types.h"
#include "model/CharacterData.h"

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

	struct CardMetaData
	{
		enum class Flag
		{
			Imported	= 1 << 0,
			Hidden		= 1 << 1,
			Favorite	= 1 << 2,
		};
		using Flags = EnumFlags<Flag>;

		fig::string name; // Not serialized
		fig::timestamp createdAt {}; // Not serialized
		fig::timestamp updatedAt {}; // Not serialized
		fig::timestamp lastUsedAt {}; // Not serialized
		uint32_t chatCount {}; // Not serialized
		CharacterGender gender {}; // Not serialized

		CardBorderStyle borderStyle {};
		Flags flags {};

		static std::optional<CardMetaData> FromJson(const fig::string& json);
		static fig::string ToJson(const CardMetaData& metaData);

		bool IsNew() const noexcept;
	};

	using CardMetaDataRef = std::reference_wrapper<CardMetaData>;
}

#endif