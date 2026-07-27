#pragma once

#include "Figment.h"
#include "data/CharacterGender.h"

namespace fig::io
{
	struct ContentMetaData
	{
		fig::string name;
		fig::uuid parentId {};
		AssetType assetType {};
		uint8_t assetSubtype {};
		fig::timestamp createdAt {};
		fig::timestamp updatedAt {};
		fig::timestamp lastUsedAt {};
		uint32_t chatCount {};
		fig::data::Gender gender {};
		fig::string_list tags {};

		bool IsNew() const noexcept;
	};
}
