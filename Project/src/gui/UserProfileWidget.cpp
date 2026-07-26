#include <pch.h>
#include "gui/UserProfileWidget.h"
#include "gui/ImageWithMask.h"
#include "gui/AppResources.h"
#include "gui/ButtonWithIcon.h"
#include "gui/MainFrame.h"
#include "gui/CustomRenderers.h"

namespace fig::gui
{
	UserProfileWidget::UserProfileWidget(ControlPtr pParent) noexcept : Panel(pParent)
	{
		_pImage = CreateControl<ImageWithMask>(nullptr, nullptr);
		_pImage->SetSize(48, 48);

		_pLabel = CreateControl<StaticText>("", FontFace::Default, 14.0, false);
		_pLabel->EnableEllipsis(true);

		_pSignOutButton = CreateControl<ButtonWithIcon>(Resource::ICON_LOGOUT);
		_pSignOutButton->SetTheme(Theme::SidePanelButtonStyle);
		_pSignOutButton->SetSize(36, 36);
		_pSignOutButton->CenterVertically();
		_pSignOutButton->SetDelegate([]() { MainFrame::GetInstance().SignOut(); });

		SetBorderRenderer<LineBorderRenderer>(Color::LineColor, Direction::North);
	}

	void UserProfileWidget::SetUser(const fig::user::UserProfile& profile) noexcept
	{
		if (_pImage)
		{
			if (auto pTexture = AppResources::GetUserProfileImage(GetSDLRenderer(), profile); pTexture)
				_pImage->SetTexture(pTexture, AppResources::GetTexture(Resource::MASK_CIRCLE));
			else
				_pImage->SetTexture(AppResources::GetTexture(Resource::PROFILE_DEFAULT_IMAGE), AppResources::GetTexture(Resource::MASK_CIRCLE));
		}

		if (_pLabel)
			_pLabel->SetText(profile.name);
	}

	void UserProfileWidget::OnSize()
	{
		if (_pImage)
		{
			_pImage->SetX(8);
			_pImage->CenterVertically();
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
	}

	void UserProfileWidget::Reset()
	{
		_pImage->SetTexture(nullptr, nullptr);
		_pLabel->Reset();
	}
}