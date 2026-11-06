#include <pch.h>
#include "gui/editor/StoryEditor.h"
#include "gui/editor/StoryEditorInfoTab.h"
#include "gui/editor/StoryEditorRolesTab.h"
#include "gui/editor/StoryEditorScenesTab.h"
#include "gui/editor/StoryEditorImagesTab.h"
#include "gui/editor/StoryEditorRulesTab.h"
#include "gui/editor/StoryEditorAboutTab.h"
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
		Roles,
		Scenes,
//		Locations,
		Images,
		Rules,
//		Concepts,
		About,
	};

	StoryEditor::StoryEditor(ControlPtr pParent) : Editor(pParent)
	{
		CreateTab<StoryEditorInfoTab>();
		CreateTab<StoryEditorRolesTab>();
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
		_scenario.name = "Untitled story";
		_scenario.roles.emplace_back(Scenario::RoleSlot {
			.role = Role::Bot1,
		});
		_scenario.roles.emplace_back(Scenario::RoleSlot {
			.role = Role::User,
			.flags { Scenario::RoleSlot::Flag::User },
		});

		_scenario.scenes.emplace_back(Scenario::Scene {
			.id = _CreateUUID(),
			.name = "Introduction",
			.content = "{character} joins {user} for a casual conversation.",
			.events = std::vector<Scenario::Event> {
				Scenario::Event {
					.type = Scenario::Event::Type::Instruct,
					.content = "{character} begins the conversation by introducing {themself} to {user}.",
				},
			},
			.ttl = 15,
			.bEntryPoint = true,
		});

		StoryEditorArgs args
		{
			.assetId = _assetId,
			.pScenario = &_scenario,
		};

		InitTabs(args);
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
			};

			InitTabs(args);
			return true;
		}

		_assetId = {};
		return false;
	}

	bool StoryEditor::EditCharacterStory(const fig::uuid& characterId) noexcept
	{
		bool bFound = false;

		fig::observer_ptr<const Character> pCharacter {};
		if (auto try_character = Global::GetUserContent().Get<fig::data::Character>(characterId))
			pCharacter.reset(&try_character.value());
		else
			return false; // Not found

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
			.pCharacter = pCharacter,
		};

		InitTabs(args);
		return true;
	}

	void StoryEditor::PopulateTopBar(ControlPtr pParent)
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
				Tab::General,
				"Info",
				Icon::STORY_EDIT_INFO,
				Icon::STORY_EDIT_INFO_SMALL,
			},
			{
				Tab::Images,
				"Images",
				Icon::CHARACTER_EDIT_IMAGES,
				Icon::CHARACTER_EDIT_IMAGES_SMALL,
			},
			{
				Tab::Roles,
				"Roles",
				Icon::STORY_EDIT_ROLES,
				Icon::STORY_EDIT_ROLES_SMALL,
			},
			{
				Tab::Scenes,
				"Scenes",
				Icon::STORY_EDIT_SCENES,
				Icon::STORY_EDIT_SCENES_SMALL,
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

	void StoryEditor::OnPropertyChanged()
	{
		_pSaveButton->SetTheme(ButtonThemes::SaveButton);
	}

	void StoryEditor::SaveChanges()
	{
		if (Save())
		{
			PushEvent(UserEvent::NavigateToStories);
			return;
		}

		auto& dlg = CreateDialogBox("An error occurred while saving.", DialogBoxType::Ok);
		dlg.Show();
	}
}