#include <pch.h>
#include "gui/CharacterEditorTraitsTab.h"
#include "gui/TextBox.h"
#include "gui/ComboBox.h"
#include "gui/ButtonWithLabel.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/ToggleWithLabel.h"
#include "gui/CharacterAttributeWidget.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "gui/Menu.h"
#include "gui/GridSizer.h"
#include "data/Character.h"
#include "io/FileUtility.h"

using namespace fig::data;
using namespace fig::io;

namespace fig::gui
{
	static constexpr size_t kMaxTraitCount = 10uz;
	static constexpr fig::point kTraitToggleSize { 129, 35 };

	CharacterEditorTraitsTab::CharacterEditorTraitsTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);

		_traitsInfo.LoadFromCsv(fig::path { "resources/editor/traits.csv" });
	}

	bool CharacterEditorTraitsTab::Initialize(CharacterEditorArgs args)
	{
		if (not (bool)args.pCharacter)
			return false;

		_pCharacter = args.pCharacter;

		auto pSizer = SetSizer<VerticalSizer>();

		// Traits
		_traitsLabel = CreateHeader(this, pSizer, "Traits");
		for (auto& group : _traitsInfo.groups)
		{
			auto pLabel = CreateBoldLabel(this, pSizer, group.name);
			_traitGroupLabels[group.name] = pLabel;

			auto pTraitGridSizer = new GridSizer(kTraitToggleSize.x, kTraitToggleSize.y, 8, 6);
			pTraitGridSizer->SetMaxColumns(5);
			pSizer->AddSpacer(2);
			pSizer->Add(pTraitGridSizer, 0, SizerFlag::Expand | SizerFlag::Bottom, 8);

			for (auto& trait : group.traits)
			{
				auto pTrait = CreateTrait(pTraitGridSizer, trait.id, trait.name);
				pTrait->SetOn(_pCharacter->HasTrait(trait.id), true);
			}
		}
		RefreshToggleGroupLabels();

		return true;
	}

	void CharacterEditorTraitsTab::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}

	fig::observer_ptr<ToggleWithLabel> CharacterEditorTraitsTab::CreateTrait(SizerPtr pSizer, fig::handle traitId, fig::string_view label)
	{
		auto pToggle = CreateControl<ToggleWithLabel>(label, 14.5, ToggleBehavior::Default);
		pToggle->SetDelegate([this, traitId](bool bOn) { OnToggledTrait(traitId, bOn); });
		pToggle->SetSize(kTraitToggleSize.x, kTraitToggleSize.y);
		pSizer->Add(pToggle, 0, SizerFlag::Right, 8);
		_traitToggles[traitId] = pToggle;
		return pToggle;
	}

	void CharacterEditorTraitsTab::OnToggledTrait(const fig::handle& traitId, bool bOn)
	{
		if (bOn)
		{
			for (auto& group : _traitsInfo.groups)
			{
				for (auto& trait : group.traits)
				{
					if (trait.id == traitId)
					{
						_pCharacter->SetTrait(traitId, trait.name, trait.text, trait.visibility);
						break;
					}
				}
			}
		}
		else
		{
			_pCharacter->RemoveTrait(traitId);
		}

		RefreshToggleGroupLabels();
	}

	void CharacterEditorTraitsTab::RefreshToggleGroupLabels()
	{
		if constexpr (Disabled)
		{
			auto traitIds = _pCharacter->GetTraits()
				| std::views::transform([](auto&& t) { return t.id; })
				| std::ranges::to<std::unordered_set>();

			for (auto& group : _traitsInfo.groups)
			{
				if (auto pLabel = _traitGroupLabels[group.name])
				{
					size_t count = 0uz;
					for (auto& trait : group.traits)
					{
						if (traitIds.contains(trait.id))
							++count;
					}

					pLabel->SetText(std::format("{} ({})", group.name, count));
				}
			}
		}
		else
		{
			_traitsLabel->SetText(std::format("Traits ({} of {})", _pCharacter->GetTraits().size(), kMaxTraitCount));

			if (_pCharacter->GetTraits().size() >= kMaxTraitCount)
			{
				for (auto& kvp : _traitToggles)
					kvp.second->SetEnabled(kvp.second->IsOn());
			}
			else
			{
				for (auto& kvp : _traitToggles)
				{
					if (not kvp.second->GetEnabled())
						kvp.second->SetEnabled(true);
				}
			}

		}
	}

}