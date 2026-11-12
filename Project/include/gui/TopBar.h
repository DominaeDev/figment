#pragma once

#include "gui/Panel.h"

namespace fig::gui
{
	class ScrollPanel;
	class TopBar : public Panel
	{
	public:
		using InitializeDelegate = std::function<void(fig::observer_ptr<TopBar>, SizerPtr)>;

		TopBar(ControlPtr pParent, InitializeDelegate fnInit = nullptr);
		TopBar(ControlPtr pParent, fig::observer_ptr<ScrollPanel> pScrollPanel, InitializeDelegate fnInit = nullptr);
	
		void Initialize();

		void SetTitle(fig::string_view text) noexcept;

	protected:
		void OnUpdate(float) override;
		void OnSize() override;

	private:
		InitializeDelegate _fnInit;
		fig::observer_ptr<StaticText> _pTitle;
		fig::observer_ptr<VerticalGradient> _pShadow;
		fig::observer_ptr<ScrollPanel> _pScrollPanel;

	};
}