#pragma once

#include "gui/Panel.h"

namespace fig::gui
{
	class StaticText;
	class PlayButton;

	class LoadModelWidget : public Panel
	{
	public:
		LoadModelWidget(LayoutElement* pParent) noexcept;

		void Reset();

	protected:
		void OnSize() override;
		EventResult OnEvent(Event& event) override;
		void SetProgress(float fProgress);
		void OnButtonPressed();

	private:
		PlayButton* _pLoadButton {};
		StaticText* _pLabel {};
		ButtonWithIcon* _pSettingsButton {};
		Panel* _pProgressBar {};
		float _fProgress {};
	};
}
