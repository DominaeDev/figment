#pragma once

#include "Figment.h"
#include "data/CharacterGender.h"

namespace fig::io
{
	struct ContentMetaData
	{
		fig::string name;
		fig::uuid parentId {};
		AssetTypeDefinition assetType {};
		fig::timestamp createdAt {};
		fig::timestamp updatedAt {};
		fig::timestamp lastUsedAt {};
		
		// Character
		fig::data::Gender gender {};
		fig::string_list tags {};
		bool hasVoice {};

		bool IsNew() const noexcept;
	};
}
