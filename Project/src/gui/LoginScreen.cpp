#include <pch.h>
#include "gui/LoginScreen.h"
#include "gui/AppResources.h"
#include "gui/VerticalSizer.h"
#include "gui/PasswordBox.h"
#include "gui/MainFrame.h"
#include "model/AppState.h"
#include "model/UserManager.h"

namespace fig::gui
{
	LoginScreen::LoginScreen(Frame* pParent) : Screen(pParent)
	{
		auto pLogo = new Image(this, AppResources::GetTexture(TextureType::LOGO_SMALL), Colors::Black);
		pLogo->SetPosition(44, 0);

		auto pMenuButton = new ButtonWithIcon(this, TextureType::ICON_MENU);
		pMenuButton->SetTheme(Themes::SidePanelButtonStyle);
		pMenuButton->SetSize(36, 36);
		pMenuButton->SetPosition(4, 6);

		// Center
		auto center = new Panel(this);
//		center->SetBackgroundColor(Colors::White);
		center->SetSize(500, 400);

		auto pHorizontalSizer = new HorizontalSizer();
		pHorizontalSizer->AddStretchSpacer();
		pHorizontalSizer->Add(center, 0, Sizer::Expand);
		pHorizontalSizer->AddStretchSpacer();

		auto pVerticalSizer = new VerticalSizer();
		pVerticalSizer->AddStretchSpacer();
		auto centerArea = pVerticalSizer->Add(pHorizontalSizer, this, 0, Sizer::Expand);
		centerArea->SetSize(500, 260);
		pVerticalSizer->AddStretchSpacer();

		auto pProfileImage = new ImageWithMask(center, nullptr, nullptr);
		pProfileImage->SetSize(160, 160);

		auto pProfileName = new StaticText(center, "", FontFace::Default, 24.0, true);
		pProfileName->SetAlignment(TextAlignment::Middle_Top);

		auto pPassword = new PasswordBox(center);
		pPassword->SetEnterPressedCallback([this](fig::string password) { TrySignIn(password); });
		pPassword->SetFocus(true);

		auto pCenterSizer = new VerticalSizer();
		pCenterSizer->Add(pProfileImage, 0, Sizer::AlignCenterHorizontal, 0);
		pCenterSizer->AddSpacer(4);
		pCenterSizer->Add(pProfileName, 0, Sizer::Expand, 0);
		pCenterSizer->AddSpacer(28);
		pCenterSizer->Add(pPassword, 0, Sizer::AlignCenterHorizontal, 0);
		center->SetSizer(pCenterSizer);

		SetSizer(pVerticalSizer);

		auto& userMngr = Global::GetUserManager();
		auto& profile = userMngr.GetProfiles().front();

		pProfileName->SetTextAndResize(profile.name);

		if (auto pTexture = AppResources::GetUserProfileImage(GetSDLRenderer(), profile); pTexture)
			pProfileImage->SetTexture(pTexture, AppResources::GetTexture(TextureType::PROFILE_MASK));
		else
			pProfileImage->SetTexture(AppResources::GetTexture(TextureType::PROFILE_DEFAULT_IMAGE), AppResources::GetTexture(TextureType::PROFILE_MASK));

		// Create profile pic
		if constexpr (Debugging and Disabled)
		{
			if (userMngr.SignInDefaultProfile())
			{
				auto& assets = userMngr.GetProfileAssets();
				assets.CreateProfilePicture(userMngr.GetActiveProfile(), fig::path("./import/profile_pic.png"));
			}
		}
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

	bool LoginScreen::TrySignIn(const fig::string& password)
	{
		auto& userMngr = Global::GetUserManager();
		auto& profile = userMngr.GetProfiles().front();

		auto startTime = std::chrono::steady_clock::now();

		if (userMngr.SignIn(profile.id, ""))
		{
			MainFrame::GetInstance().OnSignedIn(profile);

			auto endTime = std::chrono::steady_clock::now();
			MainFrame::SetStatusBar(std::format("Duration: {}ms", toD(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count())));
			return true;
		}

		return false;
	}
}