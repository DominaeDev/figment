#pragma once

#include "Figment.h"
#include "io/Error.h"
#include "data/Character.h"
#include "data/Scenario.h"
#include <expected>

namespace fig::io
{
	class CardImporter
	{
	public:
		static void ShowImportDialog(fig::window_ptr pWindow) noexcept;
		static std::expected<std::pair<fig::data::Character, fig::data::Scenario>, FileError> Import(fig::path filename) noexcept;
	};
}
