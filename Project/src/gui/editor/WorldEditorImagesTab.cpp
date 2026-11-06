#include <pch.h>
#include "gui/editor/WorldEditorImagesTab.h"
#include "gui/editor/CharacterPortraitWidget.h"
#include "gui/editor/CharacterBackgroundWidget.h"
#include "gui/HorizontalLine.h"
#include "gui/GridSizer.h"
#include "gui/Menu.h"
#include "gui/AppResources.h"
#include "gui/ButtonWithIcon.h"
#include "data/World.h"
#include "io/ContentManager.h"
#include "io/FileUtility.h"

using namespace fig::io;
using namespace fig::data;

namespace fig::gui
{
	WorldEditorImagesTab::WorldEditorImagesTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool WorldEditorImagesTab::OnInit(WorldEditorArgs args)
	{
		if (not (bool)args.pWorld)
			return false;

		auto pSizer = SetSizer<VerticalSizer>();
		_worldId = args.assetId;

		CreateHeader(this, pSizer, "Cover image");
		InitCover(pSizer);
		CreateHorizontalLine(this, pSizer);

		CreateHeader(this, pSizer, "Backgrounds");
		InitBackgrounds(pSizer);
		CreateHint(this, pSizer, std::format("You can add up to {} background images", Constants::GUI::CharacterEditor::MaxBackgrounds));

		_removedAssets.clear();
		return true;
	}

	void WorldEditorImagesTab::OnUpdate(float fElapsed)
	{
		// Load images in load queue
		ProcessLoadQueue();
	}

	void SDLCALL WorldEditorImagesTab::OnFileDialogResult(void* userdata, const char* const* fileList, int filter)
	{
		if (not fileList)
			return;

		auto pArgs = static_cast<FileDialogUserData*>(userdata);
		for (const char* const* path = fileList; *path; ++path)
			pArgs->pThis->OnOpenFile(std::filesystem::u8path(*path), pArgs->type);
	}

	void WorldEditorImagesTab::OpenFile(ImageType type)
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
		SDL_ShowOpenFileDialog(OnFileDialogResult, (void*)&_fileDlgUserData, GetSDLWindow(), filters, SDL_arraysize(filters), nullptr, type != ImageType::Cover);
	}

	void WorldEditorImagesTab::OnOpenFile(const fig::path& filename, ImageType type)
	{
		switch (type)
		{
		case ImageType::Cover:
			_coverLoadQueue.push(filename);
			break;
		case ImageType::Background:
			_backgroundLoadQueue.push(filename);
			break;
		}
	}

	void WorldEditorImagesTab::InitCover(SizerPtr pSizer)
	{
		_pCoverGridSizer = new GridSizer(Constants::GUI::CharacterEditor::PortraitWidth + 12, Constants::GUI::CharacterEditor::PortraitHeight + 35, 4, 4);
		pSizer->Add(_pCoverGridSizer.get());

		// Find cover asset
		if (auto try_cover = Global::GetUserContent().GetAssets().FindAssetOfType(make_asset_type(AssetType::Image, ImageAssetType::CoverImage), _worldId))
		{
			_coverAssetId = (*try_cover).GetMeta<fig::uuid>(MetaTag::ReferenceToOriginal).value_or({});
			
			auto pWidget = CreateControl<CharacterPortraitWidget>();
			pWidget->SetImage(_coverAssetId);
			pWidget->SetDoubleClickDelegate([this] { OpenFile(ImageType::Cover); });
			pWidget->SetRightClickDelegate([this, pWidget] { OnClickedCover(pWidget, SDL_BUTTON_RIGHT); });
			_pCoverGridSizer->Add(pWidget);

			_coverWidgets.emplace_back(ImageWidget {
				.assetId = _coverAssetId,
				.pControl = pWidget,
			});
		}
		else
			_coverAssetId = {};

		// Add cover
		auto pAddButton = CreateControl<ButtonWithIcon>(Icon::PLUS, true);
		pAddButton->SetSize(Constants::GUI::CharacterEditor::PortraitWidth, Constants::GUI::CharacterEditor::PortraitHeight);;
		pAddButton->SetDelegate([this] { OpenFile(ImageType::Cover); });
		pAddButton->SetVisible(_coverWidgets.empty());
		_pAddCoverButton = pAddButton;
		_pCoverGridSizer->Add(_pAddCoverButton, 0, SizerFlag::All, 6);
	}

	void WorldEditorImagesTab::InitBackgrounds(SizerPtr pSizer)
	{
		_pBackgroundGridSizer = new GridSizer(Constants::GUI::CharacterEditor::BackgroundWidth + 12, Constants::GUI::CharacterEditor::BackgroundHeight + 12, 4, 4);
		pSizer->Add(_pBackgroundGridSizer.get());

		auto backgroundAssets = Global::GetUserContent().GetAssets().FindAssetsOfType(make_asset_type(AssetType::Image, ImageAssetType::Background), _worldId);
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
		auto pAddButton = CreateControl<ButtonWithIcon>(Icon::PLUS, true);
		pAddButton->SetSize(Constants::GUI::CharacterEditor::BackgroundWidth, Constants::GUI::CharacterEditor::BackgroundHeight);
		pAddButton->SetDelegate([this] { OpenFile(ImageType::Background); });
		_pAddBackgroundButton = pAddButton;
		_pBackgroundGridSizer->Add(_pAddBackgroundButton, 0, SizerFlag::All, 6);
	}
	
	void WorldEditorImagesTab::OnClickedCover(ControlPtr pControl, int32_t button)
	{
		size_t index;
		if (auto itFind = std::ranges::find(_coverWidgets, pControl, [](auto&& p) { return p.pControl; }); itFind != std::ranges::cend(_coverWidgets))
			index = std::distance(_coverWidgets.begin(), itFind);
		else
			return; 
		
		if (button == SDL_BUTTON_RIGHT)
		{
			auto& menu = CreateMenu();
			menu.AddItem("Remove")
				.SetDelegate([this, index] { RemoveCover(index); });
			menu.Show();
		}
	}

	void WorldEditorImagesTab::OnClickedBackground(ControlPtr pControl)
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

	void WorldEditorImagesTab::RemoveCover(size_t index)
	{
		if (index >= _coverWidgets.size())
			return;

		auto& cover = _coverWidgets[index];

		if (not cover.assetId.empty())
			_removedAssets.push_back(cover.assetId);

		_pCoverGridSizer->Remove(cover.pControl);
		DestroyChild(cover.pControl);
		_coverWidgets.erase(_coverWidgets.cbegin() + index);

		_pAddCoverButton->SetVisible(_coverWidgets.empty());
		InvalidateLayout();
		SetDirty();
	}

	void WorldEditorImagesTab::RemoveBackground(size_t index)
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
		SetDirty();
	}

	void WorldEditorImagesTab::MoveBackgroundUp(size_t index)
	{
		if (index <= 0)
			return;

		std::swap(_backgroundWidgets[index - 1], _backgroundWidgets[index]);

		_pBackgroundGridSizer->RemoveAll();
		for (auto& background : _backgroundWidgets)
			_pBackgroundGridSizer->Add(background.pControl);
		_pBackgroundGridSizer->Add(_pAddBackgroundButton, 0, SizerFlag::All, 6);
		InvalidateLayout();
		SetDirty();
	}

	void WorldEditorImagesTab::MoveBackgroundDown(size_t index)
	{
		if (index + 1 >= _backgroundWidgets.size())
			return;

		std::swap(_backgroundWidgets[index], _backgroundWidgets[index + 1]);

		_pBackgroundGridSizer->RemoveAll();
		for (auto& widget : _backgroundWidgets)
			_pBackgroundGridSizer->Add(widget.pControl);
		_pBackgroundGridSizer->Add(_pAddBackgroundButton, 0, SizerFlag::All, 6);
		InvalidateLayout();
		SetDirty();
	}

	void WorldEditorImagesTab::ProcessLoadQueue()
	{
		if (_coverLoadQueue.empty() and _backgroundLoadQueue.empty())
			return;

		bool bChanged = false;

		// Load covers
		while (not _coverLoadQueue.empty())
		{
			auto& path = _coverLoadQueue.front();
			auto data = fig::io::ReadFile(path).value_or({});
			if (auto try_surface = LoadImageFromMemory(data); try_surface.has_value() and not data.empty())
			{
				RemoveCover(0uz); // Remove existing

				fig::sdl::Surface surface = std::move(try_surface).value();
				auto pWidget = CreateControl<CharacterPortraitWidget>();
				pWidget->SetImage(surface);
				pWidget->SetDoubleClickDelegate([this] { OpenFile(ImageType::Cover); });
				pWidget->SetRightClickDelegate([this, pWidget] { OnClickedCover(pWidget, SDL_BUTTON_RIGHT); });
				_coverWidgets.emplace_back(ImageWidget {
					.pControl = pWidget,
					.image = std::move(surface),
					.format = DataFormatFromExt(path),
					.data = std::move(data),
				});

				_pCoverGridSizer->Insert(_pCoverGridSizer->size() - 1uz, pWidget);
				bChanged = true;
			}
			
			queue_clear(_coverLoadQueue);
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
		{
			_pAddCoverButton->SetVisible(_coverWidgets.empty());
			_pAddBackgroundButton->SetVisible(_backgroundWidgets.size() < Constants::GUI::CharacterEditor::MaxBackgrounds);
			LayoutNow();
			SetDirty();
		}
	}

	EditorTabBase::SaveResult WorldEditorImagesTab::OnSave() noexcept
	{
		auto& content = Global::GetUserContent();
		auto& assets = content.GetAssets();

		if (_coverWidgets.empty())
		{
			// Remove cover assets
			if (auto try_cover = Global::GetUserContent().GetAssets().FindAssetOfType(make_asset_type(AssetType::Image, ImageAssetType::CoverImage), _worldId))
			{
				_removedAssets.push_back((*try_cover).id);
				_removedAssets.push_back((*try_cover).GetMeta<fig::uuid>(MetaTag::ReferenceToOriginal).value_or({}));
			}
		}

		// Remove assets
		for (auto& id : _removedAssets)
			content.DeleteAsset(id);

		// Write new covers
		for (auto& cover : _coverWidgets)
		{
			if (cover.assetId.empty() and not cover.data.empty())
				cover.assetId = content.CreateLargePortrait(_worldId, cover.format, cover.data);
		}

		// Update cover
		if (not _coverWidgets.empty())
		{
			if (_coverWidgets.front().assetId != _coverAssetId)
			{
				content.ReplaceCoverImage(_worldId, _coverWidgets.front().assetId);
				_coverAssetId = _coverWidgets.front().assetId;
			}
		}

		// Write new backgrounds
		for (auto& background : _backgroundWidgets)
		{
			if (background.assetId.empty() and not background.data.empty())
				background.assetId = content.CreateBackground(_worldId, background.format, background.data);
		}

		// Assign order to backgrounds
		content.AssignOrder(_backgroundWidgets
			| std::views::transform([](auto&& w) { return w.assetId; })
			| std::ranges::to<std::vector>()
		);

		return {};
	}
}