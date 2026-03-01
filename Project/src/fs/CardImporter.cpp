#include <pch.h>
#include <json.hpp>
#include "fs/CardImporter.h"
#include "fs/FileUtility.h"
#include "fs/TavernCardV2.h"

using namespace fig::io::data;
using json = nlohmann::json;

namespace fig::io
{
	std::expected<CharacterData, FileError> CardImporter::Import(fig::path filename) noexcept
	{
		auto try_import = fig::io::ReadPNGMeta(filename, "chara", true);
		if (not try_import.has_value())
			return std::unexpected(try_import.error());

		TavernCardV2 card;
		card.Parse(try_import.value());
		
		CharacterData character;
		character.fullName = card.data.name;
		character.description = card.data.description;
		character.tags = card.data.tags;
		return character;
	}
}