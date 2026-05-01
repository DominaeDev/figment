#ifndef LOAD_MODEL_WIDGET_H__
#define LOAD_MODEL_WIDGET_H__
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
		bool OnEvent(Event& event) override;
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

#endif
