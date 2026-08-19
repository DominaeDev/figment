#pragma once

#include "gui/Panel.h"

namespace fig::gui
{
	class ScrollPanel;
	class TopBar : public Panel
	{
	public:
		TopBar(ControlPtr pParent, fig::string_view title);
		TopBar(ControlPtr pParent, fig::string_view title, fig::observer_ptr<ScrollPanel> pScrollPanel);
	
		void Initialize(fig::string_view title);
		void SetTitle(fig::string_view text) noexcept;

	protected:
		void OnUpdate(float) override;
		void OnSize() override;

	private:
		fig::observer_ptr<StaticText> _pTitle;
		fig::observer_ptr<ScrollPanel> _pScrollPanel;
		fig::observer_ptr<VerticalGradient> _pShadow;

	};
}