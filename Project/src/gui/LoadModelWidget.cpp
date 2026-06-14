#include <pch.h>
#include "gui/LoadModelWidget.h"
#include "gui/ImageWithMask.h"
#include "gui/AppResources.h"
#include "gui/ButtonWithIcon.h"
#include "gui/PlayButton.h"
#include "gui/MainFrame.h"
#include "gui/CustomRenderers.h"
#include "app/AppState.h"
#include "llm/LLMBackend.h"

namespace fig::gui
{
	constexpr int32_t ProgressBarHeight = 5;

	LoadModelWidget::LoadModelWidget(LayoutElement* pParent) noexcept : Panel(pParent)
	{
		_pProgressBar = new Panel(this);
		_pProgressBar->SetBackgroundRenderer(new SolidFillRenderer(Color { 0x57caff, 0xff }));

		_pLoadButton = new PlayButton(this);
		_pLoadButton->SetDelegate(std::bind(&LoadModelWidget::OnButtonPressed, this));

		_pLabel = new StaticText(this, toStr(fig::strings::LoadModelWidget::ModelUnloaded), FontFace::Default, 14.0, false);
		_pLabel->SetHeight(20);
		_pLabel->EnableEllipsis(true);

		_pSettingsButton = new ButtonWithIcon(this, TextureType::ICON_SETTINGS);
		_pSettingsButton->SetTheme(Themes::SidePanelButtonStyle);
		_pSettingsButton->SetSize(36, 36);
		_pSettingsButton->CenterVertically();

		SetBorderRenderer(new LineBorderRenderer(Colors::LineColor, { Direction::North }));
	}

	void LoadModelWidget::OnSize()
	{
		if (_pLoadButton)
		{
			_pLoadButton->SetX(14);
			_pLoadButton->CenterVertically();
		}

		if (_pLabel)
		{
			_pLabel->SetX(66);
			_pLabel->CenterVertically();
			_pLabel->SetMaxSize(GetWidth() - _pLabel->GetX() - 34, -1);
		}

		if (_pSettingsButton)
		{
			_pSettingsButton->SetX(GetWidth() - _pSettingsButton->GetWidth() - 4);
			_pSettingsButton->CenterVertically();
		}

		SetProgress(_fProgress);
	}

	void LoadModelWidget::Reset()
	{
		_pLabel->Reset();
	}

	EventResult LoadModelWidget::OnEvent(Event& event)
	{
		if (IsUserEvent(event, UserEvent::LLMModelLoading))
		{
			_pLabel->SetText(toStr(fig::strings::LoadModelWidget::ModelLoading));
			_pLoadButton->SetIconState(PlayButton::IconState::Spinner);
			SetProgress(0.0f);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMModelLoadingProgress))
		{
			_pLabel->SetText(std::format("{} {}%", fig::strings::LoadModelWidget::ModelLoading, event.user.code));
			_pLoadButton->SetIconState(PlayButton::IconState::Spinner);
			SetProgress(toF(event.user.code) / 100.0f);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMModelLoaded))
		{
			_pLabel->SetText(toStr(fig::strings::LoadModelWidget::ModelLoaded));
			_pLoadButton->SetIconState(PlayButton::IconState::Stop);
			SetProgress(0.0f);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMModelUnloaded))
		{
			_pLoadButton->SetIconState(PlayButton::IconState::Play);
			_pLabel->SetText(toStr(fig::strings::LoadModelWidget::ModelUnloaded));
			SetProgress(0.0f);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMModelLoadFailure))
		{
			_pLoadButton->SetIconState(PlayButton::IconState::Play);
			_pLabel->SetText(toStr(fig::strings::LoadModelWidget::ModelError));
			SetProgress(0.0f);
			return EventResult::Continue;
		}
		return EventResult::Pass;
	}

	void LoadModelWidget::SetProgress(float fProgress)
	{
		_fProgress = fProgress;
		_pProgressBar->SetSize(toI(toF(GetWidth()) * fProgress), ProgressBarHeight);
		_pProgressBar->SetY(GetHeight() - _pProgressBar->GetHeight());
		_pProgressBar->SetVisible(fProgress > 0.0f);
	}

	void LoadModelWidget::OnButtonPressed()
	{
		auto& backend = Global::GetLLMBackend();

		if (not (backend.IsInitialized() or backend.IsInitializing()))
		{
			MainFrame::GetInstance().InitializeModel();
		}
		else if (backend.IsInitialized())
		{
			MainFrame::GetInstance().UnloadModel();
		}
	}
}