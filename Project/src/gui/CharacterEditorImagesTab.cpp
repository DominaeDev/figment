#include <pch.h>
#include "gui/CharacterEditorImagesTab.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "gui/CharacterPortraitWidget.h"
#include "gui/CharacterBackgroundWidget.h"
#include "gui/CharacterSmallPortraitWidget.h"
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
	CharacterEditorImagesTab::CharacterEditorImagesTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool CharacterEditorImagesTab::Initialize(CharacterEditorArgs args)
	{
		if (not (bool)args.pCharacter)
			return false;

		auto pSizer = SetSizer<VerticalSizer>();
		_characterId = args.assetId;
		
		CreateHeader(this, pSizer, "Chat icon");
		InitSmallPortrait(pSizer);
		CreateHorizontalLine(this, pSizer);

		CreateHeader(this, pSizer, "Character portraits");
		InitPortraits(pSizer);
		CreateHint(this, pSizer, std::format("You may add up to {} portrait images", Constants::GUI::CharacterEditor::MaxPortraits));
		CreateHorizontalLine(this, pSizer);

		CreateHeader(this, pSizer, "Backgrounds");
		InitBackgrounds(pSizer);
		CreateHint(this, pSizer, std::format("You may add up to {} background images", Constants::GUI::CharacterEditor::MaxBackgrounds));

		_removedAssets.clear();
		return true;
	}

	void CharacterEditorImagesTab::OnUpdate(float fElapsed)
	{
		// Load images in load queue
		ProcessLoadQueue();
	}

	void CharacterEditorImagesTab::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}

	void SDLCALL CharacterEditorImagesTab::OnFileDialogResult(void* userdata, const char* const* fileList, int filter)
	{
		if (not fileList)
			return;

		auto pArgs = static_cast<FileDialogUserData*>(userdata);
		for (const char* const* path = fileList; *path; ++path)
			pArgs->pThis->OnOpenFile(std::filesystem::u8path(*path), pArgs->type);
	}

	void CharacterEditorImagesTab::OpenFile(CharacterImageType type)
	{
		static constexpr SDL_DialogFileFilter filters[] =
		{
			{ "Image files", "bmp;gif;jpg;jpeg;jfif;png;webp" },
			{ "BMP files", "bmp" },
			{ "GIF files", "gif" },
			{ "JPEG files", "jpg;jpeg;jfif" },
			{ "PNG files", "png" },
			{ "WEBP files", "webp" },
		};

		_fileDlgUserData.pThis = this;
		_fileDlgUserData.type = type;
		SDL_ShowOpenFileDialog(OnFileDialogResult, (void*)&_fileDlgUserData, GetSDLWindow(), filters, SDL_arraysize(filters), nullptr, type != CharacterImageType::SmallPortrait);
	}

	void CharacterEditorImagesTab::OnOpenFile(const fig::path& filename, CharacterImageType type)
	{
		switch (type)
		{
		case CharacterImageType::SmallPortrait:
			_smallPortraitLoadQueue.push(filename);
			break;
		case CharacterImageType::Portrait:
			_portraitLoadQueue.push(filename);
			break;
		case CharacterImageType::Background:
			_backgroundLoadQueue.push(filename);
			break;
		}
	}

	void CharacterEditorImagesTab::InitSmallPortrait(SizerPtr pSizer)
	{
		auto imageAssets = Global::GetUserContent().GetAssets().FindAssetsOfType(make_asset_type(AssetType::Image, ImageAssetType::SmallPortrait), _characterId);
		std::ranges::sort(imageAssets, std::ranges::less(), [](auto&& a) { return a.get().GetOrder(); });

		_pSmallPortrait = CreateControl<CharacterSmallPortraitWidget>();
		_pSmallPortrait->SetBackgroundTexture(AppResources::GetTexture(Resource::SQUARE_BACKGROUND_DEFAULT));

		_pSmallPortrait->SetRightClickDelegate([this]() { OnClickedSmallPortrait(); });
		pSizer->Add(_pSmallPortrait);

		if (not imageAssets.empty())
			_pSmallPortrait->SetImage(imageAssets[0].get().id);
	}

	void CharacterEditorImagesTab::InitPortraits(SizerPtr pSizer)
	{
		_pPortraitGridSizer = new GridSizer(Constants::GUI::CharacterEditor::PortraitWidth + 12, Constants::GUI::CharacterEditor::PortraitHeight + 35, 4, 4);
		pSizer->Add(_pPortraitGridSizer.get());

		// Find cover asset
		if (auto try_cover = Global::GetUserContent().GetAssets().FindAssetOfType(make_asset_type(AssetType::Image, ImageAssetType::CoverImage), _characterId))
			_coverAssetId = (*try_cover).GetMeta<fig::uuid>(MetaTag::ReferenceToOriginal).value_or({});
		else
			_coverAssetId = {};

		auto imageAssets = Global::GetUserContent().GetAssets().FindAssetsOfType(make_asset_type(AssetType::Image, ImageAssetType::LargePortrait), _characterId);
		std::ranges::sort(imageAssets, std::ranges::less(), [](auto&& a) { return a.get().GetOrder(); });

		for (size_t index = 0; index < imageAssets.size(); ++index)
		{
			auto& imageAssetId = imageAssets[index].get().id;

			auto pWidget = CreateControl<CharacterPortraitWidget>();
			pWidget->SetImage(imageAssetId);
			pWidget->SetDelegate([this, pWidget] { OnClickedPortrait(pWidget, SDL_BUTTON_LEFT); });
			pWidget->SetRightClickDelegate([this, pWidget] { OnClickedPortrait(pWidget, SDL_BUTTON_RIGHT); });
			_pPortraitGridSizer->Add(pWidget);

			bool isCover = imageAssetId == _coverAssetId;

			_portraitWidgets.emplace_back(ImageWidget {
				.assetId = imageAssetId,
				.pControl = pWidget,
				.isCover = isCover,
			});

			pWidget->SetSelected(isCover);
		}

		// Add portrait
		auto pAddButton = CreateControl<AddImageButton>("Add image");
		pAddButton->SetDelegate([this] { OpenFile(CharacterImageType::Portrait); });
		_pAddPortraitButton = pAddButton;
		_pPortraitGridSizer->Add(_pAddPortraitButton, 0, SizerFlag::All, 6);
	}

	void CharacterEditorImagesTab::InitBackgrounds(SizerPtr pSizer)
	{
		_pBackgroundGridSizer = new GridSizer(Constants::GUI::CharacterEditor::BackgroundWidth + 12, Constants::GUI::CharacterEditor::BackgroundHeight + 12, 4, 4);
		pSizer->Add(_pBackgroundGridSizer.get());

		auto backgroundAssets = Global::GetUserContent().GetAssets().FindAssetsOfType(make_asset_type(AssetType::Image, ImageAssetType::Background), _characterId);
		std::ranges::sort(backgroundAssets, std::ranges::less(), [](auto&& a) { return a.get().GetOrder(); });

		for (size_t index = 0; index < backgroundAssets.size(); ++index)
		{
			auto& imageAssetId = backgroundAssets[index].get().id;

			auto pWidget = CreateControl<CharacterBackgroundWidget>();
			pWidget->SetImage(imageAssetId);
			pWidget->SetRightClickDelegate([this, pWidget] { OnClickedBackground(pWidget); });
			_pBackgroundGridSizer->Add(pWidget);

			_backgroundWidgets.emplace_back(ImageWidget {
				.assetId = imageAssetId,
				.pControl = pWidget,
			});
		}

		// Add background
		auto pAddButton = CreateControl<AddImageButton>("Add image");
		pAddButton->SetSize(Constants::GUI::CharacterEditor::BackgroundWidth, Constants::GUI::CharacterEditor::BackgroundHeight);
		pAddButton->SetDelegate([this] { OpenFile(CharacterImageType::Background); });
		_pAddBackgroundButton = pAddButton;
		_pBackgroundGridSizer->Add(_pAddBackgroundButton, 0, SizerFlag::All, 6);
	}

	void CharacterEditorImagesTab::OnClickedPortrait(ControlPtr pControl, int32_t button)
	{
		size_t index;
		if (auto itFind = std::ranges::find(_portraitWidgets, pControl, [](auto&& p) { return p.pControl; }); itFind != std::ranges::cend(_portraitWidgets))
			index = std::distance(_portraitWidgets.begin(), itFind);
		else
			return; 
		
		if (button == SDL_BUTTON_LEFT)
		{
			SelectCover(index);
		}
		else if (button == SDL_BUTTON_RIGHT)
		{
			auto& menu = CreateMenu();
			menu.AddCheckItem("Use as cover", _portraitWidgets[index].isCover)
				.SetDelegate([this, index] { SelectCover(index); })
				.SetEnabled(not _portraitWidgets[index].isCover);
			menu.AddCheckItem("Use as chat icon", false)
				.SetDelegate([this, index] { SetSmallPortrait(index); });
			menu.AddSeparator();
			menu.AddItem("Move up")
				.SetDelegate([this, index] { MovePortraitUp(index); })
				.SetEnabled(index > 0);
			menu.AddItem("Move down")
				.SetDelegate([this, index] { MovePortraitDown(index); })
				.SetEnabled(index + 1 < _portraitWidgets.size());
			menu.AddSeparator();
			menu.AddItem("Remove")
				.SetDelegate([this, index] { RemovePortrait(index); })
				.SetEnabled(_portraitWidgets.size() > 1);
			menu.Show();
		}
	}

	void CharacterEditorImagesTab::OnClickedBackground(ControlPtr pControl)
	{
		size_t index;
		if (auto itFind = std::ranges::find(_backgroundWidgets, pControl, [](auto&& p) { return p.pControl; }); itFind != std::ranges::cend(_backgroundWidgets))
			index = std::distance(_backgroundWidgets.begin(), itFind);
		else
			return;

		auto& menu = CreateMenu();
		menu.AddItem("Move up")
			.SetDelegate([this, index] { MoveBackgroundUp(index); })
			.SetEnabled(index > 0);
		menu.AddItem("Move down")
			.SetDelegate([this, index] { MoveBackgroundDown(index); })
			.SetEnabled(index + 1 < _backgroundWidgets.size());
		menu.AddSeparator();
		menu.AddItem("Remove")
			.SetDelegate([this, index] { RemoveBackground(index); });
		menu.Show();
	}

	void CharacterEditorImagesTab::SelectCover(size_t index)
	{
		for (size_t i = 0; i < _portraitWidgets.size(); ++i)
		{
			_portraitWidgets[i].isCover = (i == index);
			dynamic_cast<CharacterPortraitWidget*>(_portraitWidgets[i].pControl.get())->SetSelected(i == index);
		}
	}

	void CharacterEditorImagesTab::RemovePortrait(size_t index)
	{
		if (index >= _portraitWidgets.size())
			return;

		auto& portrait = _portraitWidgets[index];
		bool wasCover = portrait.isCover;

		if (not portrait.assetId.empty())
			_removedAssets.push_back(portrait.assetId);

		_pPortraitGridSizer->Remove(portrait.pControl);
		DestroyChild(portrait.pControl);
		_portraitWidgets.erase(_portraitWidgets.cbegin() + index);

		if (wasCover && _portraitWidgets.size() > 0uz)
			SelectCover(0uz);
		InvalidateLayout();
	}

	void CharacterEditorImagesTab::MovePortraitUp(size_t index)
	{
		if (index <= 0)
			return;

		std::swap(_portraitWidgets[index - 1], _portraitWidgets[index]);

		_pPortraitGridSizer->RemoveAll();
		for (auto& portrait : _portraitWidgets)
			_pPortraitGridSizer->Add(portrait.pControl);
		_pPortraitGridSizer->Add(_pAddPortraitButton, 0, SizerFlag::All, 6);
		InvalidateLayout();
	}

	void CharacterEditorImagesTab::MovePortraitDown(size_t index)
	{
		if (index + 1 >= _portraitWidgets.size())
			return;

		std::swap(_portraitWidgets[index], _portraitWidgets[index + 1]);

		_pPortraitGridSizer->RemoveAll();
		for (auto& portrait : _portraitWidgets)
			_pPortraitGridSizer->Add(portrait.pControl);
		_pPortraitGridSizer->Add(_pAddPortraitButton, 0, SizerFlag::All, 6);
		InvalidateLayout();
	}

	void CharacterEditorImagesTab::RemoveBackground(size_t index)
	{
		if (index >= _backgroundWidgets.size())
			return;

		auto& background = _backgroundWidgets[index];

		if (not background.assetId.empty())
			_removedAssets.push_back(background.assetId);

		_pBackgroundGridSizer->Remove(background.pControl);
		DestroyChild(background.pControl);
		_backgroundWidgets.erase(_backgroundWidgets.cbegin() + index);

		InvalidateLayout();
	}

	void CharacterEditorImagesTab::MoveBackgroundUp(size_t index)
	{
		if (index <= 0)
			return;

		std::swap(_backgroundWidgets[index - 1], _backgroundWidgets[index]);

		_pBackgroundGridSizer->RemoveAll();
		for (auto& background : _backgroundWidgets)
			_pBackgroundGridSizer->Add(background.pControl);
		_pBackgroundGridSizer->Add(_pAddBackgroundButton, 0, SizerFlag::All, 6);
		InvalidateLayout();
	}

	void CharacterEditorImagesTab::MoveBackgroundDown(size_t index)
	{
		if (index + 1 >= _backgroundWidgets.size())
			return;

		std::swap(_backgroundWidgets[index], _backgroundWidgets[index + 1]);

		_pBackgroundGridSizer->RemoveAll();
		for (auto& widget : _backgroundWidgets)
			_pBackgroundGridSizer->Add(widget.pControl);
		_pBackgroundGridSizer->Add(_pAddBackgroundButton, 0, SizerFlag::All, 6);
		InvalidateLayout();
	}

	void CharacterEditorImagesTab::SetSmallPortrait(size_t index)
	{
		if (index >= _portraitWidgets.size())
			return;

		auto& portrait = _portraitWidgets[index];
		if (not portrait.image.empty())
		{
			_pSmallPortrait->SetImage(portrait.image);
			_bEditingSmallPortrait = true;
		}
		else if (not portrait.assetId.empty())
		{
			_pSmallPortrait->SetImage(portrait.assetId);
			_bEditingSmallPortrait = true;
		}
		_pSmallPortrait->ResetTransform();
	}

	void CharacterEditorImagesTab::OnClickedSmallPortrait()
	{
		auto& menu = CreateMenu();
		menu.AddItem("Load image\u2026")
			.SetDelegate([this] { OpenFile(CharacterImageType::SmallPortrait); });
		menu.AddSeparator();
		menu.AddItem("Revert")
			.SetDelegate([this] { RevertSmallPortrait(); })
			.SetEnabled(_bEditingSmallPortrait);
		
		menu.Show();
	}

	void CharacterEditorImagesTab::RevertSmallPortrait()
	{
		auto imageAssets = Global::GetUserContent().GetAssets().FindAssetsOfType(make_asset_type(AssetType::Image, ImageAssetType::SmallPortrait), _characterId);
		std::ranges::sort(imageAssets, std::ranges::less(), [](auto&& a) { return a.get().GetOrder(); });

		if (not imageAssets.empty())
		{
			_pSmallPortrait->SetImage(imageAssets[0].get().id);
			_bEditingSmallPortrait = false;
		}

	}

	void CharacterEditorImagesTab::ProcessLoadQueue()
	{
		if (_smallPortraitLoadQueue.empty() 
			and _portraitLoadQueue.empty() 
			and _backgroundLoadQueue.empty())
			return;

		bool bChanged = false;

		// Load small portrait
		if (not _smallPortraitLoadQueue.empty())
		{
			auto& path = _smallPortraitLoadQueue.front();
			auto data = fig::io::ReadFile(path).value_or({});
			if (auto try_surface = LoadImageFromMemory(data); try_surface.has_value() and not data.empty())
			{
				_pSmallPortrait->SetImage(*try_surface);
				_bEditingSmallPortrait = true;
				bChanged = true;
			}
			queue_clear(_smallPortraitLoadQueue);
		}

		// Load portraits
		while (not _portraitLoadQueue.empty())
		{
			if (_portraitWidgets.size() < Constants::GUI::CharacterEditor::MaxPortraits)
			{
				auto& path = _portraitLoadQueue.front();
				auto data = fig::io::ReadFile(path).value_or({});
				if (auto try_surface = LoadImageFromMemory(data); try_surface.has_value() and not data.empty())
				{
					fig::sdl::Surface surface = std::move(try_surface).value();
					auto pWidget = CreateControl<CharacterPortraitWidget>();
					pWidget->SetImage(surface);

					size_t index = _portraitWidgets.size();
					pWidget->SetDelegate([this, pWidget] { OnClickedPortrait(pWidget, SDL_BUTTON_LEFT); });
					pWidget->SetRightClickDelegate([this, pWidget] { OnClickedPortrait(pWidget, SDL_BUTTON_RIGHT); });
					_portraitWidgets.emplace_back(ImageWidget {
						.pControl = pWidget,
						.image = std::move(surface),
						.isCover = false,
						.format = DataFormatFromExt(path),
						.data = std::move(data),
					});

					_pPortraitGridSizer->Insert(_pPortraitGridSizer->size() - 1uz, pWidget);
					bChanged = true;
				}
			}
			_portraitLoadQueue.pop();
		}

		// Load backgrounds
		while (not _backgroundLoadQueue.empty())
		{
			if (_backgroundWidgets.size() < Constants::GUI::CharacterEditor::MaxBackgrounds)
			{
				auto& path = _backgroundLoadQueue.front();

				auto data = fig::io::ReadFile(path).value_or({});
				if (auto try_surface = LoadImageFromMemory(data); try_surface.has_value() and not data.empty())
				{
					fig::sdl::Surface surface = std::move(try_surface).value();
					auto pWidget = CreateControl<CharacterBackgroundWidget>();
					pWidget->SetImage(surface);

					size_t index = _backgroundWidgets.size();
					pWidget->SetRightClickDelegate([this, pWidget] { OnClickedBackground(pWidget); });
					_backgroundWidgets.emplace_back(ImageWidget {
						.pControl = pWidget,
						.image = std::move(surface),
						.format = DataFormatFromExt(path),
						.data = std::move(data),
					});

					_pBackgroundGridSizer->Insert(_pBackgroundGridSizer->size() - 1uz, pWidget);
					bChanged = true;
				}
			}
			_backgroundLoadQueue.pop();
		}

		if (bChanged)
			LayoutNow();
	}

	EditorTabBase::SaveResult CharacterEditorImagesTab::OnSave() noexcept
	{
		auto& content = Global::GetUserContent();
		auto& assets = content.GetAssets();

		// Remove assets
		for (auto& id : _removedAssets)
			content.DeleteAsset(id);

		// Write new portraits
		for (auto& portrait : _portraitWidgets)
		{
			if (not portrait.data.empty() and portrait.assetId.empty())
			{
				auto& asset = assets.CreateAsset(make_asset_type(AssetType::Image, ImageAssetType::LargePortrait, portrait.format), portrait.data, _characterId);
				portrait.assetId = asset.id;
			}
		}

		// Write new backgrounds
		for (auto& background : _backgroundWidgets)
		{
			if (not background.data.empty() and background.assetId.empty())
			{
				auto& asset = assets.CreateAsset(make_asset_type(AssetType::Image, ImageAssetType::Background, background.format), background.data, _characterId);
				background.assetId = asset.id;
			}
		}

		// Update cover
		if (not _portraitWidgets.empty())
		{
			if (auto itCover = std::ranges::find_if(_portraitWidgets, [](auto&& p) { return p.isCover; }); itCover != std::ranges::end(_portraitWidgets))
			{
				size_t idxCover = std::distance(_portraitWidgets.begin(), itCover);
				if (_portraitWidgets[idxCover].assetId != _coverAssetId)
					content.ReplaceCoverImage(_characterId, _portraitWidgets[idxCover].assetId);
			}
		}

		// Update small portrait
		if (_bEditingSmallPortrait)
		{
			auto smallPortrait = _pSmallPortrait->GetImage();
			if (not smallPortrait.empty()
				and smallPortrait.get()->w == Constants::Data::SmallPortraitWidth
				and smallPortrait.get()->h == Constants::Data::SmallPortraitHeight)
			{
				content.ReplaceSmallPortrait(_characterId, smallPortrait, {}); //! @todo: original asset
			}
		}

		// Assign order to portraits
		content.AssignOrder(_portraitWidgets
			| std::views::transform([](auto&& w) { return w.assetId; })
			| std::ranges::to<std::vector>()
		);

		// Assign order to backgrounds
		content.AssignOrder(_backgroundWidgets
			| std::views::transform([](auto&& w) { return w.assetId; })
			| std::ranges::to<std::vector>()
		);

		return {};
	}
}