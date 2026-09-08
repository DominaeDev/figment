#include <pch.h>
#include "gui/CharacterEditorImagesPage.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "gui/PreviewCardImage.h"
#include "data/Character.h"
#include "io/ContentManager.h"

using namespace fig::data;

namespace fig::gui
{
	CharacterEditorImagesPage::CharacterEditorImagesPage(ControlPtr pParent) : EditorPage(pParent)
	{
	}

	bool CharacterEditorImagesPage::Initialize(CharacterEditorArgs args)
	{
		if (not (bool)args.pCharacter)
			return false;

		auto pSizer = SetSizer<VerticalSizer>();
		_characterId = args.assetId;
		
		CreateHeader(this, pSizer, "Portraits");

		if (auto try_portrait = Global::GetUserContent().GetLargePortraitForCharacter(_characterId))
		{
			auto pImage = CreateControl<PreviewCardImage>();
			pImage->SetImage((*try_portrait).id);
			pSizer->Add(pImage);
		}


		return true;
	}

	void CharacterEditorImagesPage::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}

	bool CharacterEditorImagesPage::Save()
	{
		return true;
	}
}