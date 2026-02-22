#include <pch.h>
#include "gui/HomeFrame.h"
#include "gui/CharacterCard.h"
#include "gui/ScenarioCard.h"
#include "gui/GridSizer.h"
#include "gui/ScrollPanel.h"
#include "gui/MainFrame.h"
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

		auto pTopBar = new Panel(this);
		pTopBar->SetHeight(48);

		_pMainArea = new ScrollPanel(this);

		auto topSizer = new VerticalSizer();
		topSizer->Add(pTopBar, 0, Sizer::Expand);
		topSizer->Add(_pMainArea, -1, Sizer::Expand | Sizer::Left | Sizer::Right, 6);
		SetSizer(topSizer);
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
		auto startTime = std::chrono::steady_clock::now();

		// Create cards
		auto& assets = ApplicationState::GetUserManager().GetProfileAssets();
		const auto& profileId = ApplicationState::GetUserManager().GetActiveProfile().id;

		auto gridSizer = new GridSizer(Constants::GUI::HomeScreen::CardWidth, Constants::GUI::HomeScreen::CardHeight);
		gridSizer->SetSpacing(Constants::GUI::HomeScreen::CardSpacingX, Constants::GUI::HomeScreen::CardSpacingY);
		gridSizer->EnableCentering(true);
		_pMainArea->SetSizer(gridSizer);

		// Find scenarios
		auto scenarios = assets.GetAllScenarios() | std::ranges::to<std::vector>();
		std::sort(scenarios.begin(), scenarios.end(), [](const Asset& a, const Asset& b) {return a.GetCreatedAt() < b.GetCreatedAt(); });

		for (auto& asset : scenarios)
		{
			auto pCard = new ScenarioCard(_pMainArea, asset.id);
			gridSizer->Add(pCard);
		}

		// Find characters
		auto characters = assets.GetAllCharacters() | std::ranges::to<std::vector>();
		std::sort(characters.begin(), characters.end(), [](const Asset& a, const Asset& b) {return a.GetCreatedAt() < b.GetCreatedAt(); });

		for (auto& asset : characters)
		{
			auto pCard = new CharacterCard(_pMainArea, asset.id);
			gridSizer->Add(pCard);
		}

		auto endTime = std::chrono::steady_clock::now();
		double duration = toD(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());

		MainFrame::SetStatusBar(std::format("Duration: {}ms", duration));

		InvalidateLayout();
	}
}