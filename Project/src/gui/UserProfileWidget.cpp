#include <pch.h>
#include "gui/UserProfileWidget.h"
#include "gui/NonOwningImageWithMask.h"
#include "gui/AppResources.h"
#include "gui/ButtonWithIcon.h"
#include "gui/MainFrame.h"
#include "gui/CustomRenderers.h"
#include "user/UserManager.h"

using namespace fig::user;

namespace fig::gui
{
	UserProfileWidget::UserProfileWidget(ControlPtr pParent) noexcept : Panel(pParent)
	{
		SetForegroundColor(Color::SidePanelForeground);

		_pPortrait = CreateControl<NonOwningImageWithMask>();
		_pPortrait->SetSize(48, 48);

		_pLabel = CreateControl<StaticText>("", FontFace::Default, 14.0, false);
		_pLabel->EnableEllipsis(true);

		_pSettingsButton = CreateControl<ButtonWithIcon>(Icon::USER_SETTINGS, false);
		_pSettingsButton->SetTheme(ButtonThemes::SidePanel);
		_pSettingsButton->SetSize(36, 36);
		_pSettingsButton->CenterVertically();
		_pSettingsButton->SetDelegate([this] { 
			PushEvent(UserEvent::EditUserSettings);
			PushEvent(UserEvent::SelectEditorTab, 1);
		});

		_pSignOutButton = CreateControl<ButtonWithIcon>(Icon::LOGOUT, false);
		_pSignOutButton->SetTheme(ButtonThemes::SidePanel);
		_pSignOutButton->SetSize(36, 36);
		_pSignOutButton->CenterVertically();
		_pSignOutButton->SetDelegate([]() { MainFrame::GetInstance().SignOut(); });

		SetBorderRenderer<LineBorderRenderer>(Color::Border, Direction::North);
	}

	void UserProfileWidget::SetUser(const fig::user::UserProfile& profile) noexcept
	{
		if (auto profileImage = UserManager::GetProfileImage(profile))
		{
			if (auto pTexture = SDL_CreateTextureFromSurface(GetSDLRenderer(), profileImage.get()))
			{
				_pPortrait->SetTexture(pTexture, AppResources::GetTexture(Resource::MASK_CIRCLE));
				_portraitTexture.reset(pTexture);
			}
		}
		else
		{
			_pPortrait->SetTexture(AppResources::GetTexture(Resource::PROFILE_DEFAULT_IMAGE), AppResources::GetTexture(Resource::MASK_CIRCLE));
			_portraitTexture.reset();
		}

		if (_pLabel)
			_pLabel->SetText(profile.name);
	}

	void UserProfileWidget::OnSize()
	{
		if (_pPortrait)
		{
			_pPortrait->SetX(8);
			_pPortrait->CenterVertically();
		}
		
		if (_pLabel)
		{
			_pLabel->SetPosition(66, 6);
			_pLabel->SetWidth(GetWidth() - _pLabel->GetX() - 34);
		}

		if (_pSignOutButton)
		{
			_pSignOutButton->SetX(GetWidth() - _pSignOutButton->GetWidth() - 4);
			_pSignOutButton->CenterVertically();
		}

		if (_pSettingsButton)
		{
			_pSettingsButton->SetX(GetWidth() - _pSettingsButton->GetWidth() - 44);
			_pSettingsButton->CenterVertically();
		}
	}

	void UserProfileWidget::Reset()
	{
		_pPortrait->SetTexture(nullptr, nullptr);
		_pLabel->Reset();
	}
}