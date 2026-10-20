#pragma once

#include "Figment.h"
#include "io/Error.h"
#include "data/Character.h"
#include <expected>

namespace fig::io
{
	class CardImporter
	{
	public:
		static void ShowImportDialog(fig::window_ptr pWindow) noexcept;
		static std::expected<fig::data::Character, FileError> Import(fig::path filename) noexcept;
	};
}
