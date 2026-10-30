#include <pch.h>
#include "gui/StoryEditorImagesTab.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "gui/CharacterBackgroundWidget.h"
#include "gui/NonOwningImageWithMask.h"
#include "gui/GridSizer.h"
#include "gui/ButtonWithLabel.h"
#include "gui/AddImageButton.h"
#include "gui/Menu.h"
#include "gui/TexturedBorderRenderer.h"
#include "data/Scenario.h"
#include "io/ContentManager.h"
#include "io/FileUtility.h"

using namespace fig::io;
using namespace fig::data;

namespace fig::gui
{
	StoryEditorImagesTab::StoryEditorImagesTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool StoryEditorImagesTab::OnInit(StoryEditorArgs args)
	{
		if (not (bool)args.pScenario)
			return false;

		auto pSizer = SetSizer<VerticalSizer>();
		_scenarioId = args.assetId;

		CreateHeader(this, pSizer, "Backgrounds");
		InitBackgrounds(pSizer);
		CreateHint(this, pSizer, std::format("You can add up to {} background images", Constants::GUI::CharacterEditor::MaxBackgrounds));

		_removedAssets.clear();
		return true;
	}

	void StoryEditorImagesTab::OnUpdate(float fElapsed)
	{
		// Load images in load queue
		ProcessLoadQueue();
	}

	void SDLCALL StoryEditorImagesTab::OnFileDialogResult(void* userdata, const char* const* fileList, int filter)
	{
		if (not fileList)
			return;

		auto pArgs = static_cast<FileDialogUserData*>(userdata);
		for (const char* const* path = fileList; *path; ++path)
			pArgs->pThis->OnOpenFile(std::filesystem::u8path(*path));
	}

	void StoryEditorImagesTab::OpenFile()
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
		SDL_ShowOpenFileDialog(OnFileDialogResult, (void*)&_fileDlgUserData, GetSDLWindow(), filters, SDL_arraysize(filters), nullptr, true);
	}

	void StoryEditorImagesTab::OnOpenFile(const fig::path& filename)
	{
		_backgroundLoadQueue.push(filename);
	}

	void StoryEditorImagesTab::InitBackgrounds(SizerPtr pSizer)
	{
		_pBackgroundGridSizer = new GridSizer(Constants::GUI::CharacterEditor::BackgroundWidth + 12, Constants::GUI::CharacterEditor::BackgroundHeight + 12, 4, 4);
		pSizer->Add(_pBackgroundGridSizer.get());

		auto backgroundAssets = Global::GetUserContent().GetAssets().FindAssetsOfType(make_asset_type(AssetType::Image, ImageAssetType::Background), _scenarioId);
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
		pAddButton->SetDelegate([this] { OpenFile(); });
		_pAddBackgroundButton = pAddButton;
		_pBackgroundGridSizer->Add(_pAddBackgroundButton, 0, SizerFlag::All, 6);

	}

	void StoryEditorImagesTab::OnClickedBackground(ControlPtr pControl)
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

	void StoryEditorImagesTab::RemoveBackground(size_t index)
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

	void StoryEditorImagesTab::MoveBackgroundUp(size_t index)
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

	void StoryEditorImagesTab::MoveBackgroundDown(size_t index)
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

	void StoryEditorImagesTab::ProcessLoadQueue()
	{
		if (_backgroundLoadQueue.empty())
			return;

		bool bChanged = false;

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
			LayoutNow();
			SetDirty();
		}
	}

	EditorTabBase::SaveResult StoryEditorImagesTab::OnSave() noexcept
	{
		auto& content = Global::GetUserContent();
		auto& assets = content.GetAssets();

		// Remove assets
		for (auto& id : _removedAssets)
			content.DeleteAsset(id);

		// Write new backgrounds
		for (auto& background : _backgroundWidgets)
		{
			if (background.assetId.empty() and not background.data.empty())
				background.assetId = content.CreateBackground(_scenarioId, background.format, background.data);
		}

		// Assign order to backgrounds
		content.AssignOrder(_backgroundWidgets
			| std::views::transform([](auto&& w) { return w.assetId; })
			| std::ranges::to<std::vector>()
		);

		return {};
	}
}