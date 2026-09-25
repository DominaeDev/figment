#pragma once

#include "gui/Control.h"

namespace fig::data
{
	struct PackageInfo;
}

namespace fig::gui
{
	class PackageWidget : public Control
	{
	public:
		PackageWidget(ControlPtr pParent, const fig::data::PackageInfo& package);

	protected:
		void OnSize();

	private:
		fig::uuid _packageId {};
		uint32_t _downloadId {};

		fig::observer_ptr<ButtonWithIcon> _pInstallButton;
		fig::observer_ptr<ButtonWithIcon> _pInfoButton;
		fig::observer_ptr<StaticText> _pInstalledText;
		fig::observer_ptr<StaticText> _pFileSizeText;

		fig::observer_ptr<StaticText> _pProgressText;
	};
}