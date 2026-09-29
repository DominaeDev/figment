#include <pch.h>
#include "gui/PackageWidget.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"
#include "gui/FillParentSizer.h"
#include "gui/HorizontalBar.h"
#include "io/PackageManager.h"

using namespace fig::io;

namespace fig::gui
{
	PackageWidget::PackageWidget(ControlPtr pParent, const fig::data::PackageInfo& package) : Control(pParent)
	{
		_packageId = package.id;

		SetSize(720, 112);
		SetMaxWidth(720);
		SetForegroundColor(Color::PanelForeground);
		SetBackgroundColor(Color::PanelBackground);

		auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_10PX, 16);
		pBorder->SetColor(Color::Border);

		auto pBorderSizer = SetSizer<FillParentSizer>();
		auto pSizer = new VerticalSizer();
		pBorderSizer->Add(pSizer, -1, SizerFlag::All, 8);

		auto pHorizontalSizer = new HorizontalSizer();
		pSizer->Add(pHorizontalSizer, 0, SizerFlag::FixedSize, 73);

		auto pLeftSizer = new VerticalSizer();
		auto pRightSizer = new VerticalSizer();
		pHorizontalSizer->Add(pLeftSizer, -1, SizerFlag::Left, 2);
		pHorizontalSizer->Add(pRightSizer, 0, SizerFlag::FixedSize, 100);

		auto pName = CreateControl<StaticText>(package.name, FontFace::Default, 18.0, false);
		pName->SetMaxWidth(480);
		pName->EnableEllipsis(true);
		pLeftSizer->Add(pName, 0);

		auto pVersion = CreateControl<StaticText>("", FontFace::Italic, 11.5, false);
		pVersion->SetText(std::format("Version {}", (fig::string)package.version));
		pLeftSizer->Add(pVersion, 0, SizerFlag::Top, 2);

		_pDescription = CreateControl<StaticText>("", FontFace::Default, 14.0, false);
		_pDescription->EnableWordWrap(true);
		_pDescription->SetTextAndResize(package.description);
		_pDescription->SetMaxWidth(480);
		pLeftSizer->Add(_pDescription, 0, SizerFlag::Top, 8);

		_pInstallButton = CreateControl<ButtonWithIcon>(Resource::ICON_DOWNLOAD, true);
		_pInstallButton->SetSize(68, 68);
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
		_pInfoButton->SetPosition(GetWidth() - 80 - _pInfoButton->GetWidth(), 8);
		_pInfoButton->SetDelegate([infoUrl] { SDL_OpenURL(infoUrl.c_str()); });

		_pInstalledText = CreateControl<StaticText>("", FontFace::Default, 14.0, false);
		_pInstalledText->SetTextAndResize("Installed \u2714");
		_pInstalledText->SetVisible(false);
		_pInstalledText->SetPosition(GetWidth() - _pInstalledText->GetWidth() - 10, GetHeight() - _pInstalledText->GetHeight() - 9);

		_pFileSizeText = CreateControl<StaticText>("", FontFace::Default, 14.0, false);
		_pFileSizeText->SetTextAndResize(format_file_size(package.fileSize));
		_pFileSizeText->SetVisible(false);
		_pFileSizeText->SetPosition(GetWidth() - _pFileSizeText->GetWidth() - 10, GetHeight() - _pFileSizeText->GetHeight() - 9);

		_pProgressBar = CreateControl<HorizontalBar>(Resource::HORIZONTAL_BAR);
		_pProgressBar->SetForegroundColor(Color::CardShadow);
		_pProgressBar->SetHeight(8);
		_pProgressBar->SetVisible(false);
		pSizer->Add(_pProgressBar, 0, SizerFlag::Expand | SizerFlag::FixedSize, 40);

		_pProgressFill = _pProgressBar->CreateControl<HorizontalBar>(Resource::HORIZONTAL_BAR);
		_pProgressFill->FillParent();
		_pProgressFill->SetForegroundColor(Color::ProgressBarFill);

		_pProgressText = CreateControl<StaticText>("", FontFace::Default, 11.5, false);
		_pProgressText->SetTextAndResize(std::format("{} ({}%)", format_file_size(package.fileSize), 32));
		_pProgressText->SetPosition(8, GetHeight() - _pProgressText->GetHeight() - 4);
		_pProgressText->SetVisible(false);
	}

	void PackageWidget::OnUpdate(float fElapsed)
	{

	}

	void PackageWidget::OnSize()
	{
	}

	void PackageWidget::RefreshState()
	{
		_packageState = Global::GetPackageManager().GetPackageState(_packageId);

		if (_packageState == PackageState::Installed)
		{
			_pInstallButton->SetIcon(Resource::ICON_UNINSTALL);
			_pFileSizeText->SetVisible(false);
			_pInstalledText->SetVisible(true);
			_pProgressBar->SetVisible(false);
			_pProgressText->SetVisible(false);
			_pDescription->SetVisible(true);
			return;
		}
		else
		{
			_pInstalledText->SetVisible(false);
		}

		_installationState = Global::GetPackageManager().GetInstallationState(_packageId);

		bool bShowProgressBar = _installationState.phase == InstallationPhase::Downloading
			or _installationState.phase == InstallationPhase::Decompressing
			or _installationState.phase == InstallationPhase::Verifying
			or _installationState.phase == InstallationPhase::Installing;

		if (bShowProgressBar)
		{
			_pInstallButton->SetIcon(Resource::ICON_DOWNLOAD_PAUSE);
			_pFileSizeText->SetVisible(false);
			_pProgressBar->SetVisible(true);
			_pProgressFill->SetWidth(std::max(toI(_installationState.GetProgress() * _pProgressBar->GetWidth()), 18));
			_pProgressText->SetVisible(true);
			_pDescription->SetVisible(false);
		}
		else
		{
			_pInstallButton->SetIcon(Resource::ICON_DOWNLOAD);
			_pFileSizeText->SetVisible(false);
			_pProgressBar->SetVisible(false);
			_pProgressText->SetVisible(false);
			_pDescription->SetVisible(true);
		}

		if (_installationState.phase == InstallationPhase::Downloading)
		{
			_pProgressText->SetTextAndResize(std::format("Downloading\u2026 {}/{} ({}%)",
				format_file_size(_installationState.bytesReceived),
				format_file_size(_installationState.bytesTotal),
				toI(_installationState.GetProgress() * 100.0f)
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
	}

	void PackageWidget::OnButtonClicked()
	{
		auto installationPhase = Global::GetPackageManager().GetInstallationState(_packageId).phase;
		if (installationPhase != InstallationPhase::None and (int32_t)installationPhase < (int32_t)InstallationPhase::Completed)
		{
			Global::GetPackageManager().CancelInstall(_packageId);
			RefreshState();
			return;
		}

		if (_packageState < PackageState::Installed)
		{
			Global::GetPackageManager().InstallPackage(_packageId);
			RefreshState();
		}
	}
}
