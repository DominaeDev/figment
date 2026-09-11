#include <pch.h>
#include "gui/CharacterEditorImagesPage.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "gui/CharacterPortraitImage.h"
#include "gui/GridSizer.h"
#include "gui/ButtonWithLabel.h"
#include "gui/AddImageButton.h"
#include "gui/Menu.h"
#include "data/Character.h"
#include "io/ContentManager.h"
#include "io/FileUtility.h"

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
		
		CreateHeader(this, pSizer, "Avatar");

		CreateHeader(this, pSizer, "Portrait(s)");

		_pImageGridSizer = new GridSizer(Constants::GUI::Cards::Half::Width + 12, Constants::GUI::Cards::Half::Height + 35, 4, 4);
		pSizer->Add(_pImageGridSizer.get());

		// Find portrait(s)
		if (auto try_cover = Global::GetUserContent().GetAssets().FindAssetOfType(make_asset_type(AssetType::Image, ImageAssetType::CoverImage), _characterId))
			_coverAssetId = (*try_cover).GetMeta<fig::uuid>(MetaTag::ReferenceToOriginal).value_or({});
		else
			_coverAssetId = {};

		auto images = Global::GetUserContent().GetAssets().FindAssetsOfType(make_asset_type(AssetType::Image, ImageAssetType::LargePortrait), _characterId);
		std::ranges::sort(images, [this](auto&& a, auto&& b) -> int {
			if (a.get().id == _coverAssetId)
				return -1;
			else if (b.get().id == _coverAssetId)
				return 1;
			return a.get().GetCreatedAt() < b.get().GetCreatedAt();
		});

		for (size_t index = 0; index < images.size(); ++index)
		{
			auto& image = images[index];

			auto pImage = CreateControl<CharacterPortraitImage>();
			pImage->SetImage(image.get().id);
			pImage->SetMouseDownDelegate([this, pImage](int32_t button, fig::point) { OnClickedPortrait(pImage, button); });
			_pImageGridSizer->Add(pImage);

			bool isCover = image.get().id == _coverAssetId;

			_portraits.emplace_back(Portrait {
				.assetId = image.get().id,
				.pControl = pImage,
				.order = toI(index),
				.isCover = isCover,
			});

			pImage->SetSelected(isCover);
		}

		auto pAddButton = CreateControl<AddImageButton>("Add image");
		pAddButton->SetDelegate([this] { OpenFile(); });
		_pAddButton = pAddButton;
		_pImageGridSizer->Add(pAddButton);

		CreateHeader(this, pSizer, "Background(s)");


		_removedImages.clear();
		return true;
	}

	void CharacterEditorImagesPage::OnUpdate(float fElapsed)
	{
		// Load images in load queue
		LoadImages();
	}

	void CharacterEditorImagesPage::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}

	void SDLCALL CharacterEditorImagesPage::OnFileDialogResult(void* userdata, const char* const* fileList, int filter)
	{
		if (not fileList)
			return;

		auto pThis = static_cast<CharacterEditorImagesPage*>(userdata);
		for (const char* const* path = fileList; *path; ++path)
			pThis->OnOpenFile(std::filesystem::u8path(*path));
	}

	void CharacterEditorImagesPage::OpenFile()
	{
		static constexpr SDL_DialogFileFilter filters[] =
		{
			{ "Image files", "png;jpg;jpeg" }
		};

		SDL_ShowOpenFileDialog(OnFileDialogResult, (void*)this, GetSDLWindow(), filters, SDL_arraysize(filters), nullptr, true);
	}

	void CharacterEditorImagesPage::OnOpenFile(const fig::path& filename)
	{
		_loadQueue.push(filename);
	}

	void CharacterEditorImagesPage::OnClickedPortrait(ControlPtr pControl, int32_t button)
	{
		size_t index;
		if (auto itFind = std::ranges::find(_portraits, pControl, [](auto&& p) { return p.pControl; }); itFind != std::ranges::cend(_portraits))
			index = std::distance(_portraits.begin(), itFind);
		else
			return; 
		
		if (button == SDL_BUTTON_LEFT)
		{
			SelectCover(index);
		}
		else if (button == SDL_BUTTON_RIGHT)
		{
			auto& menu = CreateMenu();
			menu.AddCheckItem("Use as cover", _portraits[index].isCover)
				.SetDelegate([this, index] { SelectCover(index); })
				.SetEnabled(not _portraits[index].isCover);
			menu.AddCheckItem("Use as avatar", false); //! @todo
			menu.AddSeparator();
			menu.AddItem("Remove")
				.SetDelegate([this, index] { RemovePortrait(index); });
			menu.Show();
		}
	}

	void CharacterEditorImagesPage::SelectCover(size_t index)
	{
		for (size_t i = 0; i < _portraits.size(); ++i)
		{
			_portraits[i].isCover = (i == index);
			_portraits[i].pControl->SetSelected(i == index);
		}
	}

	void CharacterEditorImagesPage::RemovePortrait(size_t index)
	{
		if (index >= _portraits.size())
			return;

		auto& portrait = _portraits[index];
		bool wasCover = portrait.isCover;

		if (not portrait.assetId.empty())
			_removedImages.push_back(portrait.assetId);

		_pImageGridSizer->Remove(_portraits[index].pControl);
		DestroyChild(_portraits[index].pControl);
		_portraits.erase(_portraits.cbegin() + index);

		if (wasCover && _portraits.size() > 0uz)
			SelectCover(0uz);
		InvalidateLayout();
	}

	bool CharacterEditorImagesPage::Save()
	{
		auto& content = Global::GetUserContent();
		auto& assets = content.GetAssets();

		// Remove portraits
		for (auto& id : _removedImages)
			content.DeleteAsset(id);

		// Write new portraits
		for (auto& portrait : _portraits)
		{
			if (not portrait.data.empty() and portrait.assetId.empty())
			{
				auto& asset = assets.CreateAsset(make_asset_type(AssetType::Image, ImageAssetType::LargePortrait, portrait.format), portrait.data, _characterId);
				portrait.assetId = asset.id;
			}
		}

		// Update cover
		if (not _portraits.empty())
		{
			if (auto itCover = std::ranges::find_if(_portraits, [](auto&& p) { return p.isCover; }); itCover != std::ranges::end(_portraits))
			{
				size_t idxCover = std::distance(_portraits.begin(), itCover);
				if (_portraits[idxCover].assetId != _coverAssetId)
					content.ReplaceCoverImage(_characterId, _portraits[idxCover].assetId);
			}
		}

		return true;
	}

	void CharacterEditorImagesPage::LoadImages()
	{
		if (_loadQueue.empty())
			return;

		while (not _loadQueue.empty())
		{
			auto& path = _loadQueue.front();

			auto data = fig::io::ReadFile(path).value_or({});
			if (auto try_surface = LoadImageFromMemory(data); try_surface.has_value() and not data.empty())
			{
				fig::sdl::Surface surface = std::move(try_surface).value();
				auto pImage = CreateControl<CharacterPortraitImage>();
				pImage->SetImage(surface);

				size_t index = _portraits.size();
				pImage->SetMouseDownDelegate([this, pImage](int32_t button, fig::point) { OnClickedPortrait(pImage, button); });
				_portraits.emplace_back(Portrait {
					.pControl = pImage,
					.image = std::move(surface),
					.order = toI(index),
					.isCover = false,
					.format = DataFormatFromExt(path),
					.data = std::move(data),
				});

				_pImageGridSizer->Insert(_pImageGridSizer->size() - 1, pImage);
			}
			_loadQueue.pop();

		}
		LayoutNow();
	}
}