#include <pch.h>
#include "gui/HomeFrame.h"
#include "gui/CharacterCard.h"
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

		InvalidateLayout();
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

		float x = 0.0f;
		// Find character
		auto characters = assets.GetAssets()
			| std::views::filter([&profileId](const auto& a) { return a.parent_id == profileId and a.IsOfType(AssetType::Character); })
			| std::ranges::to<std::vector>();
		std::sort(characters.begin(), characters.end(), [](const Asset& a, const Asset& b) {return a.GetCreatedAt() < b.GetCreatedAt(); });

		for (auto& asset : characters)
		{
			auto pCard = new CharacterCard(_pMainArea, asset.id);
			pCard->SetPosition(x, 0);
			x += Constants::GUI::CardWidth + 16.0f;
		}
	}

}