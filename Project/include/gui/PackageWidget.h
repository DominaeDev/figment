#pragma once

#include "gui/Control.h"
#include "io/PackageManager.h"

namespace fig::gui
{
	class HorizontalBar;

	class PackageWidget : public Control
	{
	public:
		PackageWidget(ControlPtr pParent, const fig::data::PackageInfo& package);
		void RefreshState();

	protected:
		void OnUpdate(float fElapsed) override;
		void OnSize();
		void OnButtonClicked();

	private:
		fig::uuid _packageId {};
		fig::io::PackageState _packageState {};
		fig::io::InstallationState _installationState {};

		uint32_t _downloadId {};

		fig::observer_ptr<ButtonWithIcon> _pInstallButton;
		fig::observer_ptr<ButtonWithIcon> _pInfoButton;
		fig::observer_ptr<StaticText> _pDescription;
		fig::observer_ptr<StaticText> _pInstalledText;
		fig::observer_ptr<StaticText> _pFileSizeText;

		fig::observer_ptr<HorizontalBar> _pProgressBar;
		fig::observer_ptr<HorizontalBar> _pProgressFill;
		fig::observer_ptr<StaticText> _pProgressText;
	};
}