#include <pch.h>
#include "gui/EditorUtils.h"

namespace fig::gui
{
	void CreateEditorField(ControlPtr pParent, fig::observer_ptr<Sizer> pSizer, const EditorField& field)
	{
		if (auto pHeader = std::get_if<EditorHeader>(&field))
		{
			auto pLabel = pParent->CreateControl<StaticText>(fig::string { pHeader->label }, FontFace::Default, 18.5, false);
			pSizer->Add(pLabel, 0, SizerFlag::Expand | SizerFlag::Top, 8);
		}
		else if (auto pHint = std::get_if<EditorHint>(&field))
		{
			auto pLabel = pParent->CreateControl<StaticText>(fig::string { pHint->label }, FontFace::Default, 12.5, false);
			pLabel->SetForegroundColor(Color::SidePanelForeground.WithAlpha(0.8f));
			pSizer->Add(pLabel, 0, SizerFlag::Expand | SizerFlag::Left, 2);
		}
		else if (auto pField = std::get_if<std::shared_ptr<IEditorField>>(&field))
		{
			auto pLabel = pParent->CreateControl<StaticText>(fig::string { (*pField)->GetLabel() }, FontFace::Default, 14.0, false);
			pLabel->SetForegroundColor(Color::SidePanelForeground);

			auto pControl = (*pField)->CreateControl(pParent);
			pSizer->AddSpacer(8);
			pSizer->Add(pLabel, 0, SizerFlag::Expand | SizerFlag::Left | SizerFlag::Top, 2);
			pSizer->Add(pControl, 0, SizerFlag::Expand);
		}
	}
}