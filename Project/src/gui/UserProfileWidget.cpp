#include <pch.h>
#include "gui/UserProfileWidget.h"
#include "gui/ImageWithMask.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	UserProfileWidget::UserProfileWidget(LayoutElement* pParent) noexcept : Area(pParent)
	{
		_pImage = new ImageWithMask(this, nullptr, nullptr);
		_pImage->SetX(4);
		_pImage->SetSize(48, 48);

		_pLabel = new StaticText(this, "", FontFace::Default, 16.0, false);
		_pLabel->EnableEllipsis(true);
	}

	void UserProfileWidget::SetUser(const fig::user::UserProfile& profile) noexcept
	{
		if (_pImage)
		{
			if (auto pTexture = AppResources::GetUserProfileImage(GetSDLRenderer(), profile); pTexture)
				_pImage->SetTexture(pTexture, AppResources::GetTexture(TextureType::PROFILE_MASK));
			else
				_pImage->SetTexture(AppResources::GetTexture(TextureType::PROFILE_DEFAULT_IMAGE), AppResources::GetTexture(TextureType::PROFILE_MASK));
		}

		if (_pLabel)
			_pLabel->SetText(profile.name);
	}

	void UserProfileWidget::OnSize()
	{
		if (_pImage)
			_pImage->CenterVertically();
		if (_pLabel)
		{
			_pLabel->SetPosition(58, 4);
			_pLabel->SetMaxSize(GetWidth() - _pLabel->GetX() - 4, -1);
		}
	}

}