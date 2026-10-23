#include <pch.h>
#include "gui/StoryEditor.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/DialogBox.h"
#include "gui/Frame.h"
#include "gui/AppResources.h"
#include "gui/StoryEditorScenesTab.h"
#include "gui/StoryEditorImagesTab.h"
#include "gui/StoryEditorRulesTab.h"
#include "gui/StoryEditorAboutTab.h"
#include "io/ContentManager.h"

using namespace fig::io;

namespace fig::gui
{
	constexpr enum Tab : int32_t
	{
//		Roles = 0,
		Scenes,
		Images,
		Rules,
//		Concepts,
		About,
	};

	StoryEditor::StoryEditor(ControlPtr pParent) : Editor(pParent)
	{
		CreateTab<StoryEditorScenesTab>();
		CreateTab<StoryEditorImagesTab>();
		CreateTab<StoryEditorRulesTab>();
		CreateTab<StoryEditorAboutTab>();
	}

	fig::string StoryEditor::GetTitle() const noexcept
	{
		return "Editing story";
	}

	bool StoryEditor::NewStory() noexcept
	{
		auto& asset = Global::GetUserContent().GetAssets().CreateTemporaryAsset(make_asset_type(AssetType::Scenario, DataFormat::TextXml));

		_assetId = asset.id;
		_scenario = {};
		_scenario.title = "Untitled story";

		StoryEditorArgs args
		{
			.assetId = _assetId,
			.pScenario = &_scenario,
			.bCharacterStory = false,
		};

		for (auto tab : _tabs | std::views::transform([](auto&& t) { return dynamic_cast<EditorTab<StoryEditorArgs>*>(t.get()); }))
		{
			if (not (tab and tab->Initialize(args)))
				return false;
		}

		return true;
	}

	bool StoryEditor::EditStory(const fig::uuid& assetId) noexcept
	{
		if (auto try_scenario = Global::GetUserContent().Get<fig::data::Scenario>(assetId))
		{
			_assetId = assetId;
			_scenario = fig::data::Scenario { *try_scenario };
			StoryEditorArgs args
			{
				.assetId = _assetId,
				.pScenario = &_scenario,
				.bCharacterStory = false,
			};

			for (auto tab : _tabs | std::views::transform([](auto&& t) { return dynamic_cast<EditorTab<StoryEditorArgs>*>(t.get()); }))
			{
				if (not (tab and tab->Initialize(args)))
					return false;
			}

			return true;
		}

		_assetId = {};
		return false;
	}

	bool StoryEditor::EditCharacterStory(const fig::uuid& characterId) noexcept
	{
		bool bFound = false;
		if (auto find_scenario = Global::GetUserContent().GetAssets().FindAssetOfType(make_asset_type(AssetType::Scenario), characterId))
		{
			if (auto try_scenario = Global::GetUserContent().Get<fig::data::Scenario>((*find_scenario).id))
			{
				_assetId = (*find_scenario).id;
				_scenario = fig::data::Scenario { *try_scenario };
				bFound = true;
			}
		}

		if (not bFound)
		{
			auto scenarioId = Global::GetUserContent().CreateCharacterStory(characterId);
			if (auto try_scenario = Global::GetUserContent().Get<fig::data::Scenario>(scenarioId))
			{
				_assetId = scenarioId;
				_scenario = fig::data::Scenario { *try_scenario };
			}
			else
				return false; // Failed
		}

		StoryEditorArgs args
		{
			.assetId = _assetId,
			.pScenario = &_scenario,
			.bCharacterStory = true,
		};

		for (auto tab : _tabs | std::views::transform([](auto&& t) { return dynamic_cast<EditorTab<StoryEditorArgs>*>(t.get()); }))
		{
			if (not (tab and tab->Initialize(args)))
				return false;
		}

		return true;
	}

	void StoryEditor::PopulateTopBar(ControlPtr pParent)
	{
		auto pSizer = pParent->GetSizer();

		auto pSaveButton = pParent->CreateControl<ButtonWithLabelAndIcon>("Save", Resource::ICON_SAVE);
		pSaveButton->SetSize(110, 32);
		pSaveButton->SetDelegate([this] { SaveChanges(); });
		_pSaveButton = pSaveButton;

		auto pDiscardButton = pParent->CreateControl<ButtonWithLabelAndIcon>("Cancel", Resource::ICON_DISMISS);
		pDiscardButton->SetSize(110, 32);
		pDiscardButton->SetDelegate([this] { Close(); });

		pSizer->Add(_pSaveButton, 0, SizerFlag::AlignCenterVertical);
		pSizer->Add(pDiscardButton, 0, SizerFlag::Left | SizerFlag::AlignCenterVertical, 8);
		pSizer->AddSpacer(8);
	}

	bool StoryEditor::Save() noexcept
	{
		if (_assetId.empty())
			return false;

		bool bOk = SaveTabs();

		if (not bOk)
		{
			LogLn("Error occurred while saving scenario."); //! @todo: User facing error
			return false; // Error
		}

		Global::GetUserContent().UpdateAsset(_assetId, _scenario);
		Global::GetUserContent().GetAssets().SaveNow();
		return true;
	}

	void StoryEditor::OnClose()
	{
		Global::GetUserContent().GetAssets().DiscardTemporaryAssets();
	}

	std::vector<EditorTabDescriptor> StoryEditor::GetTabDescriptors() const
	{
		static size_t NotImpl = (size_t)(-1);
		return std::vector<EditorTabDescriptor> {
			{
				NotImpl,
				"Roles",
				Resource::ICON_STORY_EDIT_ROLES,
				Resource::ICON_STORY_EDIT_ROLES_SMALL,
			},
			{
				Tab::Scenes,
				"Scenes",
				Resource::ICON_STORY_EDIT_SCENES,
				Resource::ICON_STORY_EDIT_SCENES_SMALL,
			},
			{
				Tab::Images,
				"Images",
				Resource::ICON_CHARACTER_EDIT_IMAGES,
				Resource::ICON_CHARACTER_EDIT_IMAGES_SMALL,
			},
			{
				Tab::Rules,
				"Rules",
				Resource::ICON_CHARACTER_EDIT_RULES,
				Resource::ICON_CHARACTER_EDIT_RULES_SMALL,
			},
			{
				NotImpl,
				"Concepts",
				Resource::ICON_CHARACTER_EDIT_CONCEPTS,
				Resource::ICON_CHARACTER_EDIT_CONCEPTS_SMALL,
			},
			{
				Tab::About,
				"About",
				Resource::ICON_CHARACTER_EDIT_ABOUT,
				Resource::ICON_CHARACTER_EDIT_ABOUT_SMALL,
			},
		};
	}

	void StoryEditor::OnPropertyChanged()
	{
		_pSaveButton->SetTheme(ButtonThemes::SaveButton);
	}

	void StoryEditor::SaveChanges()
	{
		if (Save())
		{
			PushEvent(UserEvent::NavigateToHome);
			return;
		}

		auto& dlg = CreateDialogBox("An error occurred while saving.", DialogBoxType::Ok);
		dlg.Show();
	}
}