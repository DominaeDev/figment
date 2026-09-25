#include <pch.h>
#include "gui/PackageWidget.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"
#include "gui/FillParentSizer.h"
#include "gui/HorizontalBar.h"
#include "io/PackageManager.h"

namespace fig::gui
{
	PackageWidget::PackageWidget(ControlPtr pParent, const fig::data::PackageInfo& package) : Control(pParent)
	{
		_packageId = package.id;

		SetSize(600, 112);
		SetMaxWidth(600);
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

		auto pDescription = CreateControl<StaticText>("", FontFace::Default, 14.0, false);
		pDescription->EnableWordWrap(true);
		pDescription->SetTextAndResize(package.description);
		pDescription->SetMaxWidth(480);
		pLeftSizer->Add(pDescription, 0, SizerFlag::Top, 8);
		pDescription->SetVisible(false);

		_pInstallButton = CreateControl<ButtonWithIcon>(Resource::ICON_DOWNLOAD_PAUSE, true);
		_pInstallButton->SetSize(68, 68);
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
		_pInfoButton->SetDelegate([infoUrl]() {
			SDL_OpenURL(infoUrl.c_str());
		});

		_pInstalledText = CreateControl<StaticText>("", FontFace::Default, 14.0, false);
		_pInstalledText->SetTextAndResize("Installed \u2714");
		_pInstalledText->SetVisible(false);
		_pInstalledText->SetPosition(GetWidth() - _pInstalledText->GetWidth() - 10, GetHeight() - _pInstalledText->GetHeight() - 9);

		_pFileSizeText = CreateControl<StaticText>("", FontFace::Default, 14.0, false);
		_pFileSizeText->SetTextAndResize(format_file_size(package.fileSize));
		_pFileSizeText->SetVisible(false);
		_pFileSizeText->SetPosition(GetWidth() - _pFileSizeText->GetWidth() - 10, GetHeight() - _pFileSizeText->GetHeight() - 9);

		auto pProgressBG = CreateControl<HorizontalBar>(Resource::HORIZONTAL_BAR);
		pProgressBG->SetForegroundColor(Color::CardShadow);
		pProgressBG->SetHeight(8);
//		pProgressBG->SetVisible(false);

		auto pProgress = pProgressBG->CreateControl<HorizontalBar>(Resource::HORIZONTAL_BAR);
		pProgress->FillParent();
		pProgress->SetForegroundColor(Color::ProgressBarFill);
		pSizer->Add(pProgressBG, 0, SizerFlag::Expand | SizerFlag::FixedSize, 40);

		_pProgressText = CreateControl<StaticText>("", FontFace::Default, 11.5, false);
		_pProgressText->SetTextAndResize(std::format("{} ({}%)", format_file_size(package.fileSize), 32));
		_pProgressText->SetPosition(GetWidth() - _pProgressText->GetWidth() - 8, GetHeight() - _pProgressText->GetHeight() - 4);
//		_pProgressText->SetVisible(false);
	}

	void PackageWidget::OnSize()
	{
	}
}
