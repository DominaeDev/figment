#pragma once

#include "gui/Panel.h"

namespace fig::gui
{
	class StaticText;
	class PlayButton;

	class LoadModelWidget : public Panel
	{
	public:
		LoadModelWidget(ControlPtr pParent) noexcept;

		void Reset();

	protected:
		void OnSize() override;
		EventResult OnEvent(fig::event& event) override;
		void SetProgress(float fProgress);
		void OnButtonPressed();

	private:
		fig::observer_ptr<PlayButton> _pLoadButton;
		fig::observer_ptr<StaticText> _pLabel;
		fig::observer_ptr<ButtonWithIcon> _pSettingsButton;
		fig::observer_ptr<Panel> _pProgressBar;
		float _fProgress { 0 };
	};
}
