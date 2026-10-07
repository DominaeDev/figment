#pragma once

#include "gui/ITextStyleProvider.h"

namespace fig::gui
{
	class DefaultTextStyleProvider : public ITextStyleProvider
	{
	public:
		DefaultTextStyleProvider(TextBase* pOwner);
	protected:
		std::vector<StyleSpan> GetStyles(fig::string_view text, size_t offset = 0uz) override;
	};
}