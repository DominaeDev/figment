#pragma once

#include "data/CharacterTrait.h"
#include "io/XmlData.h"

namespace fig::data
{
	struct CharacterTraitInfo
	{
		fig::handle id;
		fig::string name;
		fig::string text;
		CharacterTrait::Visibility visibility {};
	};

	struct CharacterTraitInfoDatabase
	{
		struct TraitGroup
		{
			fig::string name;
			std::vector<CharacterTraitInfo> traits;
		};
		std::vector<TraitGroup> groups;

		bool LoadFromCsv(const fig::path& filename);
	};
}