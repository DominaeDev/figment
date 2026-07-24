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
		UserProfileWidget(control_ptr pParent) noexcept;

		void SetUser(const fig::user::UserProfile& profile) noexcept;
		void Reset();

	protected:
		void OnSize() override;

	private:
		fig::observer_ptr<ImageWithMask> _pImage {};
		fig::observer_ptr<StaticText> _pLabel {};
		fig::observer_ptr<ButtonWithIcon> _pSignOutButton {};
	};


}