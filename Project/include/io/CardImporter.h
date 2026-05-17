#ifndef CARD_IMPORTER_H__
#define CARD_IMPORTER_H__
#pragma once

#include "Figment.h"
#include "io/FileError.h"
#include "io/data/CharacterData.h"
#include <expected>

namespace fig::io
{
	class CardImporter
	{
	public:
		static std::expected<fig::io::CharacterData, FileError> Import(fig::path filename) noexcept;
	};
}

#endif