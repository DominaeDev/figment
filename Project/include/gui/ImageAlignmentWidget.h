#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	using ImageAlignmentDelegate = std::function<void(ImageAlignment)>;

	class ImageAlignmentWidget : public Control
	{
	public:
		ImageAlignmentWidget(ControlPtr pParent);

		ImageAlignment GetAlignment() const noexcept { return _value; };
		void SetAlignment(ImageAlignment align, bool bSilent = false);

		void SetDelegate(ImageAlignmentDelegate fnDelegate) { _fnDelegate = fnDelegate; }

	protected:
		void OnSize() override;

	private:
		void CreateButton(ImageAlignment alignment);

		std::array<fig::observer_ptr<ButtonWithIcon>, 9uz> _buttons {};
		ImageAlignmentDelegate _fnDelegate;
		ImageAlignment _value {};
	};
}