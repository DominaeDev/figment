#pragma once

#include "gui/Panel.h"
#include "user/UserProfile.h"

namespace fig::user
{
	struct UserProfile;
};

namespace fig::gui
{
	class ImageWithMask;
	class StaticText;
	class ButtonWithIcon;

	class UserProfileWidget : public Panel
	{
	public:
		UserProfileWidget(LayoutElement* pParent) noexcept;

		void SetUser(const fig::user::UserProfile& profile) noexcept;
		void Reset();

	protected:
		void OnSize() override;

	private:
		ImageWithMask* _pImage {};
		StaticText* _pLabel {};
		ButtonWithIcon* _pSignOutButton {};
	};


}