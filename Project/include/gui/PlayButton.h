#pragma once

#include "gui/ThemedButton.h"

namespace fig::gui
{
	class Image;

	class PlayButton : public ThemedButton
	{
		PlayButton() = delete;

	public:
		PlayButton(LayoutElement* pParent);

		enum IconState
		{
			Play,
			Spinner,
			Stop,
		};
		void SetIconState(IconState state);

	protected:
		void OnUpdate(float fElapsed) override;
		void OnSize() override;
		void OnButtonState() override;

	private:
		fig::observer_ptr<Image> _pBackground;
		fig::observer_ptr<Image> _pIcon;
		double _spinnerAngle {};
		IconState _iconState {};
	};
}
