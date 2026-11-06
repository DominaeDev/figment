#include <pch.h>
#include "gui/editor/WorldEditor.h"
#include "gui/editor/WorldEditorInfoTab.h"
#include "gui/editor/WorldEditorImagesTab.h"
#include "gui/editor/WorldEditorRulesTab.h"
#include "gui/editor/WorldEditorAboutTab.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/DialogBox.h"
#include "gui/Frame.h"
#include "gui/AppResources.h"
#include "io/ContentManager.h"

using namespace fig::io;
using namespace fig::chat;
using namespace fig::data;

namespace fig::gui
{
	constexpr enum Tab : int32_t
	{
		General,
//		Locations,
		Images,
		Rules,
//		Concepts,
		About,
	};

	WorldEditor::WorldEditor(ControlPtr pParent) : Editor(pParent)
	{
		CreateTab<WorldEditorInfoTab>();
		CreateTab<WorldEditorImagesTab>();
		CreateTab<WorldEditorRulesTab>();
		CreateTab<WorldEditorAboutTab>();
	}

	fig::string WorldEditor::GetTitle() const noexcept
	{
		return "Editing world";
	}

	bool WorldEditor::NewWorld() noexcept
	{
		auto& asset = Global::GetUserContent().GetAssets().CreateTemporaryAsset(make_asset_type(AssetType::World, DataFormat::TextXml));

		_assetId = asset.id;
		_world = {};
		_world.name = "Untitled world";

		WorldEditorArgs args
		{
			.assetId = _assetId,
			.pWorld = &_world,
		};

		InitTabs(args);
		return true;
	}

	bool WorldEditor::EditWorld(const fig::uuid& assetId) noexcept
	{
		if (auto try_world = Global::GetUserContent().Get<fig::data::World>(assetId))
		{
			_assetId = assetId;
			_world = fig::data::World { *try_world };
			
			WorldEditorArgs args
			{
				.assetId = _assetId,
				.pWorld = &_world,
			};

			InitTabs(args);
			return true;
		}

		_assetId = {};
		return false;
	}

	void WorldEditor::PopulateTopBar(ControlPtr pParent)
	{
		auto pSizer = pParent->GetSizer();

		auto pSaveButton = pParent->CreateControl<ButtonWithLabelAndIcon>("Save", Icon::SAVE);
		pSaveButton->SetSize(110, 32);
		pSaveButton->SetDelegate([this] { SaveChanges(); });
		_pSaveButton = pSaveButton;

		auto pDiscardButton = pParent->CreateControl<ButtonWithLabelAndIcon>("Cancel", Icon::DISMISS);
		pDiscardButton->SetSize(110, 32);
		pDiscardButton->SetDelegate([this] { Close(); });

		pSizer->Add(_pSaveButton, 0, SizerFlag::AlignCenterVertical);
		pSizer->Add(pDiscardButton, 0, SizerFlag::Left | SizerFlag::AlignCenterVertical, 8);
		pSizer->AddSpacer(8);
	}

	bool WorldEditor::Save() noexcept
	{
		if (_assetId.empty())
			return false;

		bool bOk = SaveTabs();

		if (not bOk)
		{
			LogLn("Error occurred while saving world."); //! @todo: User facing error
			return false; // Error
		}

		Global::GetUserContent().UpdateAsset(_assetId, _world);
		Global::GetUserContent().GetAssets().SaveNow();
		return true;
	}

	void WorldEditor::OnClose()
	{
		Global::GetUserContent().GetAssets().DiscardTemporaryAssets();
	}

	std::vector<EditorTabDescriptor> WorldEditor::GetTabDescriptors() const
	{
		static size_t NotImpl = (size_t)(-1);
		return std::vector<EditorTabDescriptor> {
			{
				Tab::General,
				"Info",
				Icon::MENU_WORLDS,
				Icon::MENU_WORLDS_SMALL,
			},
			{
				Tab::Images,
				"Images",
				Icon::CHARACTER_EDIT_IMAGES,
				Icon::CHARACTER_EDIT_IMAGES_SMALL,
			},
			{
				NotImpl,
				"Locations",
				Icon::STORY_EDIT_LOCATIONS,
				Icon::STORY_EDIT_LOCATIONS_SMALL,
			},
			{
				Tab::Rules,
				"Rules",
				Icon::CHARACTER_EDIT_RULES,
				Icon::CHARACTER_EDIT_RULES_SMALL,
			},
			{
				NotImpl,
				"Concepts",
				Icon::CHARACTER_EDIT_CONCEPTS,
				Icon::CHARACTER_EDIT_CONCEPTS_SMALL,
			},
			{
				Tab::About,
				"About",
				Icon::CHARACTER_EDIT_ABOUT,
				Icon::CHARACTER_EDIT_ABOUT_SMALL,
			},
		};
	}

	void WorldEditor::OnPropertyChanged()
	{
		_pSaveButton->SetTheme(ButtonThemes::SaveButton);
	}

	void WorldEditor::SaveChanges()
	{
		if (Save())
		{
			PushEvent(UserEvent::NavigateToWorlds);
			return;
		}

		auto& dlg = CreateDialogBox("An error occurred while saving.", DialogBoxType::Ok);
		dlg.Show();
	}
}