#include <pch.h>
#include "gui/HomeFrame.h"
#include "gui/CharacterCard.h"
#include "gui/ScenarioCard.h"
#include "gui/GridSizer.h"
#include "model/AppState.h"
#include "model/UserManager.h"
#include "model/AssetManager.h"
#include "fs/Serialization.h"

using namespace fig::fs;

namespace fig::gui
{
	HomeFrame::HomeFrame(Frame* pParent) : Screen(pParent)
	{
		SetForegroundColor(Colors::Black);
		SetBackgroundColor(Colors::AppBackground);

		_pMainArea = new Area(this);

		auto topSizer = new VerticalSizer();
		topSizer->Add(_pMainArea, -1, Sizer::Expand);
		SetSizer(topSizer);
	}

	void HomeFrame::OnUpdate(float fDeltaTime)
	{

	}

	void HomeFrame::OnRender(Renderer* pRenderer)
	{
		DrawBackground(pRenderer);
	}

	bool HomeFrame::OnKeyboardEvent(KeyboardEvent& event)
	{
		if (event.pressed) // Press
		{
		}
		else // Release
		{
		}
		return false;
	}

	void HomeFrame::CreateCards()
	{
		// Create cards
		auto& assets = ApplicationState::GetUserManager().GetProfileAssets();
		const auto& profileId = ApplicationState::GetUserManager().GetActiveProfile().id;

		float x = 40.0f;

		auto gridSizer = new GridSizer(Constants::GUI::CardWidth, Constants::GUI::CardHeight);
		gridSizer->SetSpacing(12, 12);
		_pMainArea->SetSizer(gridSizer);

		// Find scenarios
		auto scenarios = assets.GetAllScenarios() | std::ranges::to<std::vector>();
		std::sort(scenarios.begin(), scenarios.end(), [](const Asset& a, const Asset& b) {return a.GetCreatedAt() < b.GetCreatedAt(); });

		for (auto& asset : scenarios)
		{
			auto pCard = new ScenarioCard(_pMainArea, asset.id);
			pCard->SetPosition(x, 40.0f);
			x += Constants::GUI::CardWidth + 16.0f;
			gridSizer->Add(pCard);
		}

		// Find characters
		auto characters = assets.GetAllCharacters() | std::ranges::to<std::vector>();
		std::sort(characters.begin(), characters.end(), [](const Asset& a, const Asset& b) {return a.GetCreatedAt() < b.GetCreatedAt(); });

		for (auto& asset : characters)
		{
			auto pCard = new CharacterCard(_pMainArea, asset.id);
			pCard->SetPosition(x, 40.0f);
			x += Constants::GUI::CardWidth + 16.0f;
			gridSizer->Add(pCard);
		}


		InvalidateLayout();
	}

}