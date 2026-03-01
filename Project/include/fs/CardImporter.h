#ifndef CARD_IMPORTER_H__
#define CARD_IMPORTER_H__
#pragma once

#include "Types.h"
#include "fs/FileError.h"
#include "model/CharacterData.h"
#include <expected>

namespace fig::io
{
	class CardImporter
	{
	public:
		static std::expected<fig::io::data::CharacterData, FileError> Import(fig::path filename) noexcept;
	};
}

#endif