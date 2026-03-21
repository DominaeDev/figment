#include <pch.h>
#include "gui/CardList.h"
#include "gui/GridSizer.h"
#include "gui/ScenarioCard.h"
#include "gui/CharacterCard.h"
#include "model/AppState.h"
#include "model/UserManager.h"
#include "model/AssetManager.h"
#include "util/Common.h"

using namespace fig::io;

namespace fig::gui
{
	constexpr Coord TopMargin = 8;
	constexpr Coord BottomMargin = 120;

	CardList::CardList(LayoutElement* pParent, CardSize cardSize) : ScrollPanel(pParent),
		_cardSize { cardSize }
	{
		int divide = cardSize == CardSize::Default ? 1 : 2;

		_pGridSizer = new GridSizer(Constants::GUI::HomeScreen::CardWidth / divide, Constants::GUI::HomeScreen::CardHeight / divide);
		_pGridSizer->SetSpacing(Constants::GUI::HomeScreen::CardSpacingX / divide, Constants::GUI::HomeScreen::CardSpacingY / divide);
		_pGridSizer->EnableCentering(true);
		SetTopMargin(TopMargin);
		SetBottomMargin(BottomMargin);

		SetSizer(_pGridSizer);
		EnableClipping(true);
		EnableCulling(true);
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
				auto pCard = new ScenarioCard(this, asset.id, _cardSize);
				auto request = assets.LoadAssetAsync(asset.id, AsyncTask::LoadCover, priority--);
				pCard->SetPendingCoverImage(std::move(request.future));
				_pGridSizer->Add(pCard);
				_cards.push_back(pCard);
			}
		}

		// Find characters
		if (cardType == CardType::Character)
		{
			auto characters = assets.GetAllCharacters() | std::ranges::to<std::vector>();
			std::sort(characters.begin(), characters.end(), [](const Asset& a, const Asset& b) {return a.GetCreatedAt() < b.GetCreatedAt(); });

			for (auto& asset : characters)
			{
				auto pCard = new CharacterCard(this, asset.id, _cardSize);
				_pGridSizer->Add(pCard);
				_cards.push_back(pCard);
			}
		}

		for (auto& pCard : _cards)
		{
			auto request = assets.LoadAssetAsync(pCard->GetAssetID(), AsyncTask::LoadCover, priority--);
			pCard->SetPendingCoverImage(std::move(request.future));
		}

		InvalidateLayout();
	}

	void CardList::OnUpdate(float fElapsed)
	{
		int32_t curr_rows = toI(_pGridSizer->GetRows());
		int divide = _cardSize == CardSize::Default ? 1 : 2;

		if (_last_rows != curr_rows)
		{
			auto height = GetHeight();
			auto kCardHeight = Constants::GUI::HomeScreen::CardHeight / divide;
			auto last_extent = (_last_rows * kCardHeight + std::max(_last_rows - 1, 0) * Constants::GUI::HomeScreen::CardSpacingY);
			auto curr_extent = (curr_rows * kCardHeight + std::max(curr_rows - 1, 0) * Constants::GUI::HomeScreen::CardSpacingY);

			_maxExtent = curr_extent;
			_last_rows = curr_rows;

			if (last_extent > 0)
			{
				float ratio = _fScrollY / last_extent;
				_fScrollY = ratio * toF(_maxExtent);
				LayoutNow();
			}
		}
	}

	void CardList::SetFilter(const fig::string& search_string) noexcept
	{
		if (fig::util::empty_or_whitespace(search_string))
		{
			ClearFilter();
			return;
		}

		for (auto& card : _cards)
		{
			bool bFiltered = card->IsFilteredBy(search_string);
			card->SetVisible(not bFiltered);
			card->EnableLayout(not bFiltered);
		}

		InvalidateLayout();
	}

	void CardList::ClearFilter() noexcept
	{
		for (auto& card : _cards)
		{
			card->SetVisible(true);
			card->EnableLayout(true);
		}
		InvalidateLayout();
	}

	void CardList::SetCardSize(CardSize cardSize)
	{
		if (cardSize == _cardSize)
			return;

		_cardSize = cardSize;

		for (auto& card : _cards)
			card->SetCardSize(cardSize);

		int divide = cardSize == CardSize::Default ? 1 : 2;

		_pGridSizer->SetCellSize(Constants::GUI::HomeScreen::CardWidth / divide, Constants::GUI::HomeScreen::CardHeight / divide);
		_pGridSizer->SetSpacing(Constants::GUI::HomeScreen::CardSpacingX, Constants::GUI::HomeScreen::CardSpacingY);

		_fScrollY = 0;
		InvalidateLayout();
	}
}