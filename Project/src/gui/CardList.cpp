#include <pch.h>
#include "gui/CardList.h"
#include "gui/GridSizer.h"
#include "gui/ScenarioCard.h"
#include "gui/CharacterCard.h"
#include "model/AppState.h"
#include "model/UserManager.h"
#include "model/AssetManager.h"
#include "fs/Serialization.h"

using namespace fig::io;

namespace fig::gui
{
	constexpr float BottomMargin = 120.0f;

	CardList::CardList(Control* pParent) : ScrollPanel(pParent)
	{
		_pGridSizer = new GridSizer(Constants::GUI::HomeScreen::CardWidth, Constants::GUI::HomeScreen::CardHeight);
		_pGridSizer->SetSpacing(Constants::GUI::HomeScreen::CardSpacingX, Constants::GUI::HomeScreen::CardSpacingY);
		_pGridSizer->EnableCentering(true);
		SetSizer(_pGridSizer);

		EnableClipping(true);
		EnableCulling(true);
		SetBottomMargin(BottomMargin);
	}

	void CardList::CreateCards(CardType cardType)
	{
		// Create cards
		auto& assets = Global::GetUserManager().GetProfileAssets();
		const auto& profileId = Global::GetUserManager().GetActiveProfile().id;

		RemoveChildren(true);

		int32_t priority = 0;
		if (cardType == CardType::Scenario)
		{
			// Find scenarios
			auto scenarios = assets.GetAllScenarios() | std::ranges::to<std::vector>();
			std::sort(scenarios.begin(), scenarios.end(), [](const Asset& a, const Asset& b) {return a.GetCreatedAt() < b.GetCreatedAt(); });

			for (auto& asset : scenarios)
			{
				auto pCard = new ScenarioCard(this, asset.id);
				auto request = assets.LoadAssetAsync(asset.id, AssetManager::AsyncLoad::Task::LoadImage, priority--);
				pCard->SetPendingCoverImage(std::move(request.future));
				_pGridSizer->Add(pCard);
			}
		}

		// Find characters
		if (cardType == CardType::Character)
		{
			auto characters = assets.GetAllCharacters() | std::ranges::to<std::vector>();
			std::sort(characters.begin(), characters.end(), [](const Asset& a, const Asset& b) {return a.GetCreatedAt() < b.GetCreatedAt(); });

			for (auto& asset : characters)
			{
				auto pCard = new CharacterCard(this, asset.id);
				auto request = assets.LoadAssetAsync(asset.id, AssetManager::AsyncLoad::Task::LoadImage, priority--);
				pCard->SetPendingCoverImage(std::move(request.future));
				_pGridSizer->Add(pCard);
			}
		}

		InvalidateLayout();
	}

	void CardList::OnUpdate(float fElapsed)
	{
		int32_t curr_rows = toI(_pGridSizer->GetRows());

		if (_last_rows != curr_rows)
		{
			auto height = GetHeight();
			auto last_extent = _last_rows * Constants::GUI::HomeScreen::CardHeight + std::max(_last_rows - 1, 0) * Constants::GUI::HomeScreen::CardSpacingY;
			auto curr_extent = curr_rows * Constants::GUI::HomeScreen::CardHeight + std::max(curr_rows - 1, 0) * Constants::GUI::HomeScreen::CardSpacingY;

			_fMaxExtent = toF(curr_extent);
			_last_rows = curr_rows;

			if (last_extent > 0)
			{
				float ratio = _fScrollY  / last_extent;
				_fScrollY = ratio * _fMaxExtent;
				LayoutNow();
			}
		}
	}
}