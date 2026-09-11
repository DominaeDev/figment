#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class PreviewCardImage;

	class CharacterPortraitWidget : public Control, public MouseEventHandler
	{
	public:
		CharacterPortraitWidget(ControlPtr pParent);
	
		void SetImage(const fig::uuid& assetId);
		void SetImage(const fig::sdl::Surface& surface);
		void SetSelected(bool bSelected);
		
	protected:
		void OnSize() override;
		EventResult OnEvent(fig::event& event) override;

	private:
		fig::observer_ptr<PreviewCardImage> _pPortrait;
		fig::observer_ptr<Control> _pSelection;
		fig::observer_ptr<StaticText> _pLabel;

		bool _bSelected {};
	};
}