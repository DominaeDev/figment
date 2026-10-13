#pragma once

#include "gui/Overlay.h"

namespace fig::chat
{
	class ChatSession;
}

namespace fig::gui
{
	class ChatBackground;
	class BehindChat;
	class BackgroundImageCarousel;
	class ImageAlignmentWidget;

	class ChatBackgroundPopOver : public Overlay
	{
	public:
		ChatBackgroundPopOver(FramePtr pFrame, fig::observer_ptr<fig::chat::ChatSession> pSession, fig::observer_ptr<ChatBackground> pBackground, fig::observer_ptr<BehindChat> pBehindChat);

		uint32_t Show(fig::point position = { -1, -1 });
		uint32_t Show(fig::rect parentRect);

	protected:
		EventResult OnEvent(fig::event& event) override;
		
		void EnableBackground(bool bEnable);
		void SetImage(const fig::uuid& assetId);
		void SetFit(ImageFit fit);
		void SetBounded(bool bBounded);
		void EnableControls(bool bEnabled);

		fig::observer_ptr<StaticText> CreateLabel(fig::string_view text);
		fig::observer_ptr<Slider> CreateSlider();

	private:
		fig::observer_ptr<ChatBackground> _pBackground;
		fig::observer_ptr<CheckBox> _pEnabled;
		fig::observer_ptr<Slider> _pOpacitySlider;
		fig::observer_ptr<Slider> _pSaturationSlider;
		fig::observer_ptr<Slider> _pBlurSlider;
		fig::observer_ptr<Slider> _pVeilSlider;
		fig::observer_ptr<ToggleWithLabel> _pFitInside;
		fig::observer_ptr<ToggleWithLabel> _pFitOutside;
		fig::observer_ptr<ToggleWithLabel> _pFitArea;
		fig::observer_ptr<ToggleWithLabel> _pFitWindow;
		fig::observer_ptr<ImageAlignmentWidget> _pAlignment;
		fig::observer_ptr<BackgroundImageCarousel> _pCarousel;
		fig::observer_ptr<BehindChat> _pBehindChat;
	};
}