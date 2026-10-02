#include <pch.h>
#include "gui/PackageWidget.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"
#include "gui/FillParentSizer.h"
#include "gui/HorizontalBar.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "io/PackageManager.h"

using namespace fig::io;

namespace fig::gui
{
	constexpr float RefreshCadence = 0.1f;

	PackageWidget::PackageWidget(ControlPtr pParent, const fig::data::PackageInfo& package) : Control(pParent)
	{
		_packageId = package.id;
		_packageDependencies = Condition(package.dependencies, true);
		_sizeString = format_file_size(package.fileSize);

		SetSize(720, 106);
		SetMaxWidth(720);
		SetForegroundColor(Color::PanelForeground);
		SetBackgroundColor(Color::PanelBackground);

		auto pBGRenderer = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_10PX, 16);
		pBGRenderer->SetColor(GetBackgroundColor());
		auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_10PX, 16);
		pBorder->SetColor(Color::Border);

		auto pBorderSizer = SetSizer<FillParentSizer>();
		auto pSizer = new VerticalSizer();
		pBorderSizer->Add(pSizer, -1, SizerFlag::All, 8);

		auto pHorizontalSizer = new HorizontalSizer();
		pSizer->Add(pHorizontalSizer, 0, SizerFlag::FixedSize, 62);

		auto pLeftSizer = new VerticalSizer();
		auto pRightSizer = new VerticalSizer();
		pHorizontalSizer->Add(pLeftSizer, -1, SizerFlag::Left, 2);
		pHorizontalSizer->Add(pRightSizer, 0, SizerFlag::FixedSize, 100);

		_pName = CreateControl<StaticText>("", FontFace::Default, 18.0, false);
		_pName->SetForegroundColor(Color::AppForeground);
		_pName->SetTextAndResize(package.name);
		_pName->SetMaxWidth(540);
		_pName->EnableEllipsis(true);
		pLeftSizer->Add(_pName, 0);

		auto pVersion = CreateControl<StaticText>("", FontFace::Italic, 11.5, false);
		if (not package.versionString.empty())
			pVersion->SetText(package.versionString);
		else if (package.version.is_valid())
			pVersion->SetText(std::format("Version {}", (fig::string)package.version));
		pLeftSizer->Add(pVersion, 0, SizerFlag::Top, 2);

		_pDescription = CreateControl<StaticText>("", FontFace::Default, 14.0, false);
		_pDescription->EnableWordWrap(true);
		_pDescription->SetMaxWidth(560);
		_pDescription->SetWidth(560);
		_pDescription->SetTextAndResize(package.description);
		pLeftSizer->Add(_pDescription, 0, SizerFlag::Top, 8);

		_pInstallButton = CreateControl<ButtonWithLabelAndIcon>("", Resource::ICON_DOWNLOAD);
		_pInstallButton->SetSize(120, 32);
		_pInstallButton->SetDelegate([this] { OnButtonClicked(); });
		pRightSizer->Add(_pInstallButton, 0, SizerFlag::AlignRight);

		_pInfoButton = CreateControl<ButtonWithIcon>(Resource::ICON_INFO_SMALL);
		_pInfoButton->SetSize(28, 28);
		_pInfoButton->SetTheme(ButtonTheme {
			.defaultColor	{ Color::PanelBackground, Color::ButtonDefaultForeground },
			.hoverColor		{ Color::SidePanelButtonHoverBackground, Color::ButtonHoverForeground },
			.pressedColor	{ Color::SidePanelButtonPressedBackground, Color::ButtonPressedForeground },
			.disabledColor	{ Color::DisabledButtonBackground, Color::DisabledButtonForeground },
		});

		fig::string infoUrl = package.infoUrl;
		_pInfoButton->SetVisible(not infoUrl.empty());
		_pInfoButton->SetPosition(_pName->GetX() + _pName->GetWidth() + 2, 6);
		_pInfoButton->SetDelegate([infoUrl] { SDL_OpenURL(infoUrl.c_str()); });

		_pStatusText = CreateControl<StaticText>("", FontFace::Default, 14.0, false);
		_pStatusText->SetPosition(GetWidth() - _pStatusText->GetWidth() - 10, GetHeight() - _pStatusText->GetHeight() - 7);

		_pProgressBar = CreateControl<HorizontalBar>(Resource::HORIZONTAL_BAR);
		_pProgressBar->SetForegroundColor(Color::ProgressBarBackground);
		_pProgressBar->SetHeight(8);
		_pProgressBar->SetVisible(false);
		pSizer->Add(_pProgressBar, 0, SizerFlag::Expand | SizerFlag::FixedSize, 40);

		_pProgressFill = _pProgressBar->CreateControl<HorizontalBar>(Resource::HORIZONTAL_BAR);
		_pProgressFill->FillParent();
		_pProgressFill->SetForegroundColor(Color::ProgressBarFill);

		_pProgressText = CreateControl<StaticText>("", FontFace::Default, 11.5, false);
		_pProgressText->SetPosition(8, GetHeight() - _pProgressText->GetHeight() - 6);
		_pProgressText->SetVisible(false);

		RefreshState();
	}

	void PackageWidget::OnUpdate(float fElapsed)
	{
		_fRefreshCounter += fElapsed;
		if (_fRefreshCounter > RefreshCadence)
		{
			RefreshState();
			_fRefreshCounter = 0.0f;
		}
	}

	void PackageWidget::OnSize()
	{
		if (_pStatusText)
			_pStatusText->SetX(GetWidth() - _pStatusText->GetWidth() - 10);
		if (_pInfoButton)
			_pInfoButton->SetX(_pName->GetX() + _pName->GetWidth() + 2);
	}

	void PackageWidget::RefreshState()
	{
		_fRefreshCounter = 0.0f;
		_packageState = Global::GetPackageManager().GetPackageState(_packageId);

		if (_packageState == PackageState::Installed)
		{
			_pInstallButton->SetLabel("Uninstall", Resource::ICON_DELETE);
			_pInstallButton->SetEnabled(true);
			_pProgressBar->SetVisible(false);
			_pProgressText->SetVisible(false);
			_pDescription->SetVisible(true);

			_pStatusText->SetTextAndResize(std::format("Installed ({})", _sizeString));
			_pStatusText->SetForegroundColor(Color::SuccessText);
			_pStatusText->SetX(GetWidth() - _pStatusText->GetWidth() - 10);
			_pStatusText->SetVisible(true);

			SetBackgroundColor(Color::PanelBackgroundHover);
			GetBackgroundRenderer()->SetColor(Color::PanelBackgroundHover);
			_pInfoButton->SetTheme(ButtonTheme {
				.defaultColor	{ Color::PanelBackgroundHover, Color::ButtonDefaultForeground },
				.hoverColor		{ Color::SidePanelButtonHoverBackground, Color::ButtonHoverForeground },
				.pressedColor	{ Color::SidePanelButtonPressedBackground, Color::ButtonPressedForeground },
				.disabledColor	{ Color::DisabledButtonBackground, Color::DisabledButtonForeground },
			});
			return;
		}

		SetBackgroundColor(Color::PanelBackground);
		GetBackgroundRenderer()->SetColor(Color::PanelBackground);
		_pInfoButton->SetTheme(ButtonTheme {
			.defaultColor	{ Color::PanelBackground, Color::ButtonDefaultForeground },
			.hoverColor		{ Color::SidePanelButtonHoverBackground, Color::ButtonHoverForeground },
			.pressedColor	{ Color::SidePanelButtonPressedBackground, Color::ButtonPressedForeground },
			.disabledColor	{ Color::DisabledButtonBackground, Color::DisabledButtonForeground },
		});
		_installationState = Global::GetPackageManager().GetInstallationProgress(_packageId);

		bool bShowProgressBar = _installationState.phase == InstallationPhase::Downloading
			or _installationState.phase == InstallationPhase::Decompressing
			or _installationState.phase == InstallationPhase::Verifying
			or _installationState.phase == InstallationPhase::Installing;

		if (bShowProgressBar)
		{
			_pInstallButton->SetLabel("Stop", Resource::ICON_STOP);
			_pInstallButton->SetEnabled(true);
			_pStatusText->SetVisible(false);
			_pProgressBar->SetVisible(true);
			_pProgressText->SetVisible(true);
			_pDescription->SetVisible(false);
			if (_installationState.bytesReceived > 0)
			{
				_pProgressFill->SetWidth(std::max(toI(_installationState.GetProgress() * _pProgressBar->GetWidth()), 12));
				_pProgressFill->SetVisible(true);
			}
			else
				_pProgressFill->SetVisible(false);
		}
		else
		{
			_pInstallButton->SetLabel("Download", Resource::ICON_DOWNLOAD);
			_pInstallButton->SetEnabled(_packageDependencies.Evaluate(Global::GetPackageManager().GetContext()));
			_pStatusText->SetTextAndResize(_sizeString);
			_pStatusText->SetForegroundColor(Color::PanelForeground);
			_pStatusText->SetX(GetWidth() - _pStatusText->GetWidth() - 10);
			_pStatusText->SetVisible(true);
			_pProgressBar->SetVisible(false);
			_pProgressText->SetVisible(false);
			_pDescription->SetVisible(true);
		}

		if (_installationState.phase == InstallationPhase::Downloading)
		{
			_pProgressText->SetTextAndResize(std::format("Downloading\u2026 {}/{} ({:.1f}%)",
				format_file_size(_installationState.bytesReceived),
				format_file_size(_installationState.bytesTotal),
				_installationState.GetProgress() * 100.0f
			));
		}
		else if (_installationState.phase == InstallationPhase::Decompressing)
		{
			_pProgressFill->SetWidth(_pProgressBar->GetWidth());
			_pProgressText->SetTextAndResize("Decompressing\u2026");
		}
		else if (_installationState.phase == InstallationPhase::Verifying)
		{
			_pProgressFill->SetWidth(_pProgressBar->GetWidth());
			_pProgressText->SetTextAndResize("Verifying\u2026");
		}
		else if (_installationState.phase == InstallationPhase::Installing)
		{
			_pProgressFill->SetWidth(_pProgressBar->GetWidth());
			_pProgressText->SetTextAndResize("Installing\u2026");
		}
		else if (_installationState.phase == InstallationPhase::Failed)
		{
			_pStatusText->SetTextAndResize(_installationState.errorMessage);
			_pStatusText->SetForegroundColor(Color::ErrorText);
			_pStatusText->SetX(GetWidth() - _pStatusText->GetWidth() - 10);
			_pStatusText->SetVisible(true);
		}
	}

	void PackageWidget::OnButtonClicked()
	{
		auto installationPhase = Global::GetPackageManager().GetInstallationProgress(_packageId).phase;
		
		// Cancel
		if (installationPhase != InstallationPhase::None and (int32_t)installationPhase < (int32_t)InstallationPhase::Completed)
		{
			Global::GetPackageManager().CancelInstall(_packageId);
			RefreshState();
			return;
		}

		// Install / Resume
		if (_packageState < PackageState::Installed)
		{
			Global::GetPackageManager().InstallPackage(_packageId);
			RefreshState();
			return;
		}
		
		// Uninstall
		if (_packageState == PackageState::Installed)
		{
			Global::GetPackageManager().UninstallPackage(_packageId);
			RefreshState();
			return;
		}
	}
}
