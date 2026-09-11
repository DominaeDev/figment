#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class PreviewCardImage;

	class CharacterBackgroundWidget : public Control, public MouseEventHandler
	{
	public:
		CharacterBackgroundWidget(ControlPtr pParent);

		void SetImage(const fig::uuid& assetId);
		void SetImage(const fig::sdl::Surface& surface);

	protected:
		EventResult OnEvent(fig::event& event) override;

	private:
		fig::observer_ptr<PreviewCardImage> _pImage;
	};
}