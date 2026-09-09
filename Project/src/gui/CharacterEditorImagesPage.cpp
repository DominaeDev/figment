#include <pch.h>
#include "gui/CharacterEditorImagesPage.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "gui/PreviewCardImage.h"
#include "gui/GridSizer.h"
#include "data/Character.h"
#include "io/ContentManager.h"

using namespace fig::io;
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

		auto pGridSizer = new GridSizer(Constants::GUI::Cards::Half::Width, Constants::GUI::Cards::Half::Height, 12, 12);
		pSizer->Add(pGridSizer);

		auto images = Global::GetUserContent().GetAssets().FindAssetsOfType(make_asset_type(AssetType::Image, ImageAssetType::LargePortrait), _characterId);
		for (auto image : images)
		{
			auto pImage = CreateControl<PreviewCardImage>(ImageFit::Portrait);
			pImage->SetImage(image.get().id);
			pGridSizer->Add(pImage);
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