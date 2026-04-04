#include <pch.h>
#include "gui/UserProfileWidget.h"
#include "gui/ImageWithMask.h"
#include "gui/AppResources.h"
#include "gui/ButtonWithIcon.h"
#include "gui/MainFrame.h"

namespace fig::gui
{
	UserProfileWidget::UserProfileWidget(LayoutElement* pParent) noexcept : Area(pParent)
	{
		_pImage = new ImageWithMask(this, nullptr, nullptr);
		_pImage->SetSize(48, 48);

		_pLabel = new StaticText(this, "", FontFace::Default, 14.0, false);
		_pLabel->EnableEllipsis(true);

		_pButton = new ButtonWithIcon(this, TextureType::ICON_LOGOUT);
		_pButton->SetTheme(Themes::SidePanelButtonStyle);
		_pButton->SetSize(36, 36);
		_pButton->CenterVertically();
		_pButton->SetDelegate([]() { MainFrame::GetInstance().SignOut(); });
	}

	void UserProfileWidget::SetUser(const fig::user::UserProfile& profile) noexcept
	{
		if (_pImage)
		{
			if (auto pTexture = AppResources::GetUserProfileImage(GetSDLRenderer(), profile); pTexture)
				_pImage->SetTexture(pTexture, AppResources::GetTexture(TextureType::CIRCLE_MASK));
			else
				_pImage->SetTexture(AppResources::GetTexture(TextureType::PROFILE_DEFAULT_IMAGE), AppResources::GetTexture(TextureType::CIRCLE_MASK));
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
			_pLabel->SetMaxSize(GetWidth() - _pLabel->GetX() - 34, -1);
		}

		if (_pButton)
		{
			_pButton->SetX(GetWidth() - _pButton->GetWidth() - 8);
			_pButton->CenterVertically();
		}
	}

	void UserProfileWidget::Reset()
	{
		_pImage->SetTexture(nullptr, nullptr);
		_pLabel->Reset();
	}
}