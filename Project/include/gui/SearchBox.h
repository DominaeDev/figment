#pragma once

#include "gui/TextBox.h"

namespace fig::gui
{
	class SearchBox : public TextBox
	{
	public:
		SearchBox(ControlPtr pParent);

	protected:
		void OnSize();
		EventResult OnEvent(fig::event& event);
		bool HandleMouseEvents(const fig::event& event) noexcept;
		void OnText(fig::string_view text) noexcept override;

	private:
		fig::observer_ptr<Image> _pIcon;
		fig::observer_ptr<Image> _pCross;

		bool _bMouseInside {};
		bool _bMouseDown {};
	};
}