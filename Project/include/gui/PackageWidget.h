#pragma once

#include "gui/Control.h"
#include "io/PackageManager.h"
#include "text/Condition.h"

namespace fig::gui
{
	class HorizontalBar;
	class ButtonWithLabelAndIcon;

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

		fig::observer_ptr<ButtonWithLabelAndIcon> _pInstallButton;
		fig::observer_ptr<ButtonWithIcon> _pInfoButton;
		fig::observer_ptr<StaticText> _pName;
		fig::observer_ptr<StaticText> _pDescription;
		fig::observer_ptr<StaticText> _pStatusText;

		fig::observer_ptr<HorizontalBar> _pProgressBar;
		fig::observer_ptr<HorizontalBar> _pProgressFill;
		fig::observer_ptr<StaticText> _pProgressText;

		float _fRefreshCounter {};
		Condition _packageDependencies;
		fig::string _sizeString;
	};
}