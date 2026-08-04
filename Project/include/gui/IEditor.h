#pragma once

#include "EditorFields.h"

namespace fig::gui
{
	class IEditor
	{
	public:
		virtual ~IEditor()
		{};

		virtual fig::string GetTitle() const noexcept = 0;
		virtual EditorFields GetFields() noexcept = 0;

		virtual bool SaveChanges() noexcept = 0;
	};
}