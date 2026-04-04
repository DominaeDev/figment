#include <pch.h>
#include "gui/LoginScreen.h"
#include "gui/AppResources.h"
#include "gui/VerticalSizer.h"
#include "gui/PasswordBox.h"
#include "gui/MainFrame.h"
#include "gui/TexturedBorder.h"
#include "model/AppState.h"
#include "model/UserManager.h"
#include "util/StringUtility.h"

using namespace fig::util;

namespace fig::gui
{
	LoginScreen::LoginScreen(Frame* pParent) : Screen(pParent)
	{
		// Logo
		auto pLogo = new Image(this, AppResources::GetTexture(TextureType::LOGO_SMALL), Colors::Black);
		pLogo->SetPosition(44, 0);

		// Menu button
		auto pMenuButton = new ButtonWithIcon(this, TextureType::ICON_MENU);
		pMenuButton->SetTheme(Themes::SidePanelButtonStyle);
		pMenuButton->SetSize(36, 36);
		pMenuButton->SetPosition(4, 6);

		// Center (login)
		auto pCenter = new Panel(this);
//		pCenter->SetBackgroundColor(Colors::Green);
		pCenter->SetSize(500, 260);

		auto pVerticalSizer = new VerticalSizer();
		pVerticalSizer->AddStretchSpacer();
		pVerticalSizer->Add(pCenter, 0, Sizer::AlignCenterHorizontal);
		pVerticalSizer->AddStretchSpacer();

		_pProfileImage = new ImageWithMask(pCenter, nullptr, nullptr);
		_pProfileImage->SetSize(160, 160);

		_pPrevProfileBtn = new ButtonWithIcon(pCenter, TextureType::ICON_CHEVRON_LEFT);
		_pPrevProfileBtn->SetSize(35, 56);
		_pPrevProfileBtn->SetDelegate([this]() { CycleProfile(-1); });
		_pNextProfileBtn = new ButtonWithIcon(pCenter, TextureType::ICON_CHEVRON_RIGHT);
		_pNextProfileBtn->SetSize(35, 56);
		_pNextProfileBtn->SetDelegate([this]() { CycleProfile(+1); });

		auto pProfileImageSizer = new HorizontalSizer();
		pProfileImageSizer->Add(_pPrevProfileBtn, -1, Sizer::AlignCenterVertical | Sizer::AlignRight | Sizer::Right, 16);
		pProfileImageSizer->Add(_pProfileImage, 0);
		pProfileImageSizer->Add(_pNextProfileBtn, -1, Sizer::AlignCenterVertical | Sizer::AlignLeft | Sizer::Left, 16);

		_pProfileName = new StaticText(pCenter, "", FontFace::Default, 24.0, false);
		_pProfileName->SetAlignment(TextAlignment::Middle_Top);

		auto pPasswordArea = new Panel(pCenter);
//		pPasswordArea->SetBackgroundColor(Colors::Debug);
		pPasswordArea->SetHeight(48);

		_pPassword = new PasswordBox(pPasswordArea);
		_pPassword->SetWidth(240);
		_pPassword->SetEnterPressedCallback([this](fig::string password) { TrySignIn(); });
		_pPassword->SetFocus(true);

		auto pSignInButton = new ButtonWithIcon(pPasswordArea, TextureType::ICON_ARROW_RIGHT);
		pSignInButton->SetSize(35, 35);
		pSignInButton->SetDelegate([this]() { TrySignIn(); });
		auto pSimpleBorder = new TexturedBorder(pSignInButton, AppResources::GetTexture(TextureType::CARD_BORDER), 16);
		pSimpleBorder->FillParent();
		pSimpleBorder->SetForegroundColor(Colors::SidePanelForeground);

		auto pPasswordSizer = new HorizontalSizer();
		pPasswordSizer->AddStretchSpacer();
		pPasswordSizer->Add(_pPassword, 0, Sizer::AlignCenterVertical);
		pPasswordSizer->Add(pSignInButton, 0, Sizer::AlignCenterVertical | Sizer::AlignLeft | Sizer::Left, 4);
		pPasswordSizer->AddStretchSpacer();
		pPasswordArea->SetSizer(pPasswordSizer);

		auto pCenterSizer = new VerticalSizer();
		pCenter->SetSizer(pCenterSizer);
		pCenterSizer->AddSizer(pProfileImageSizer, 0, Sizer::FixedSize, 160);
		pCenterSizer->AddSpacer(4);
		pCenterSizer->Add(_pProfileName, 0, Sizer::Expand | Sizer::FixedSize, 24);
		pCenterSizer->AddSpacer(28);
		pCenterSizer->Add(pPasswordArea, 0, Sizer::Expand);

		SetSizer(pVerticalSizer);

		auto& profiles = Global::GetUserManager().GetProfiles();
		if (auto lastProfile = Global::GetUserManager().GetProfile(Global::GetSettings().GetUUID(AppSetting::LastUser)))
			SelectProfile(lastProfile.value());
		else
		{
			if (not profiles.empty())
				SelectProfile(profiles.front());
		}

		_pPrevProfileBtn->SetVisible(profiles.size() > 1);
		_pNextProfileBtn->SetVisible(profiles.size() > 1);
	}

	void LoginScreen::OnUpdate(float fElapsed)
	{
	}

	void LoginScreen::OnRender(Renderer* pRenderer)
	{
	}

	bool LoginScreen::OnKeyboardEvent(KeyboardEvent& event)
	{
		if (event.pressed) // Press
		{
		}
		else // Release
		{
		}
		return false;
	}

	void LoginScreen::SelectProfile(const fig::user::UserProfile& profile)
	{
		_currentProfileId = profile.id;
		_pProfileName->SetText(profile.name);

		_pPassword->Clear();
		_pPassword->SetEnabled(profile.has_password);

		if (auto pTexture = AppResources::GetUserProfileImage(GetSDLRenderer(), profile); pTexture)
			_pProfileImage->SetTexture(pTexture, AppResources::GetTexture(TextureType::CIRCLE_MASK));
		else
			_pProfileImage->SetTexture(AppResources::GetTexture(TextureType::PROFILE_DEFAULT_IMAGE), AppResources::GetTexture(TextureType::CIRCLE_MASK));
	}

	void LoginScreen::CycleProfile(int32_t step)
	{
		auto& profiles = Global::GetUserManager().GetProfiles();
		if (profiles.size() < 1)
			return;

		size_t curr = 0;
		for (size_t i = 0; i < profiles.size(); ++i)
		{
			if (profiles[i].id == _currentProfileId)
			{
				curr = i;
				break;
			}
		}

		SelectProfile(profiles[toUZ((toI(curr) + step)) % profiles.size()]);
	}

	bool LoginScreen::TrySignIn()
	{
		if (auto profile = Global::GetUserManager().GetProfile(_currentProfileId))
		{
			fig::string password = trim(_pPassword->GetText());
			_pPassword->Clear();
			if (MainFrame::GetInstance().TrySignIn(profile.value(), password))
			{
				
			}
		}
		return false;
	}
}