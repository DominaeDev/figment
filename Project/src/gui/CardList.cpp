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

	static constexpr Coord cardWidth(CardSize cardSize) noexcept
	{
		return cardSize == CardSize::Default ? Constants::GUI::CardWidth : Constants::GUI::HalfCardWidth;;
	}

	static constexpr Coord cardHeight(CardSize cardSize) noexcept
	{
		return cardSize == CardSize::Default ? Constants::GUI::CardHeight: Constants::GUI::HalfCardHeight;
	}

	CardList::CardList(LayoutElement* pParent, CardSize cardSize) : ScrollPanel(pParent),
		_cardSize { cardSize }
	{
		_pGridSizer = new GridSizer(cardWidth(cardSize), cardHeight(cardSize));
		_pGridSizer->SetSpacing(Constants::GUI::CardSpacingX, Constants::GUI::CardSpacingY);
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

		Reset();

		int32_t priority = 0;
		if (cardType == CardType::Scenario)
		{
			// Find scenarios
			auto scenarios = assets.GetAllScenarios() | std::ranges::to<std::vector>();
			std::sort(scenarios.begin(), scenarios.end(), [](const Asset& a, const Asset& b) {return a.GetCreatedAt() < b.GetCreatedAt(); });

			for (auto& asset : scenarios)
			{
				DEBUG_MEASURE_BEGIN(std::format("Scenario card {}:", asset.id.str()));
				auto pCard = new ScenarioCard(this, asset.id, _cardSize);
				auto request = assets.LoadAssetAsync(asset.id, AsyncTask::LoadCover, priority--);
				pCard->SetPendingCoverImage(std::move(request.future));
				_pGridSizer->Add(pCard);
				_cards.push_back(pCard);
				DEBUG_MEASURE_END();
			}
		}

		// Find characters
		if (cardType == CardType::Character)
		{
			auto characters = assets.GetAllCharacters() | std::ranges::to<std::vector>();
			std::sort(characters.begin(), characters.end(), [](const Asset& a, const Asset& b) {return a.GetCreatedAt() < b.GetCreatedAt(); });

			for (auto& asset : characters)
			{
				DEBUG_MEASURE_BEGIN(std::format("Character card {}:", asset.id.str()));
				auto pCard = new CharacterCard(this, asset.id, _cardSize);
				_pGridSizer->Add(pCard);
				_cards.push_back(pCard);
				DEBUG_MEASURE_END();
			}
		}

		size_t initCounter = 0;
		for (auto& pCard : _cards)
		{
			auto request = assets.LoadAssetAsync(pCard->GetAssetID(), AsyncTask::LoadCover, priority--);
			pCard->SetPendingCoverImage(std::move(request.future));
			pCard->EnableTags(_bEnableTags);

			if (initCounter++ < 8)
				pCard->Init();
			else
				pCard->Cull(true);
		}

		LayoutNow();
	}

	void CardList::OnUpdate(float fElapsed)
	{
		ScrollPanel::OnUpdate(fElapsed);

		int32_t curr_rows = toI(_pGridSizer->GetRows());

		if (_last_rows != curr_rows)
		{
			auto height = GetHeight();
			auto kCardHeight = cardHeight(_cardSize);
			auto last_extent = (_last_rows * kCardHeight + std::max(_last_rows - 1, 0) * Constants::GUI::CardSpacingY);
			auto curr_extent = (curr_rows * kCardHeight + std::max(curr_rows - 1, 0) * Constants::GUI::CardSpacingY);

			_maxExtent = curr_extent;
			_last_rows = curr_rows;

			if (last_extent > 0)
			{
				float ratio = _fScrollY / last_extent;
				_fScrollY = ratio * toF(_maxExtent);
				_fTargetScrollY = _fScrollY;
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

		_pGridSizer->SetCellSize(cardWidth(cardSize), cardHeight(cardSize));
		_pGridSizer->SetSpacing(Constants::GUI::CardSpacingX, Constants::GUI::CardSpacingY);

		_fScrollY = 0;
		InvalidateLayout();
	}

	void CardList::EnableTags(bool bEnable) noexcept
	{
		if (_bEnableTags == bEnable)
			return;
		_bEnableTags = bEnable;

		for (auto& card : _cards)
			card->EnableTags(bEnable);
	}

	void CardList::Reset()
	{
		DestroyChildren();
		_cards.clear();
		_fScrollY = 0;
		_fTargetScrollY = 0;
	}
}