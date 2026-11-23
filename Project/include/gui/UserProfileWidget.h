#pragma once

#include "gui/Panel.h"
#include "user/UserProfile.h"

namespace fig::user
{
	struct UserProfile;
};

namespace fig::gui
{
	class UserProfileWidget : public Panel
	{
	public:
		UserProfileWidget(ControlPtr pParent) noexcept;

		void SetUser(const fig::user::UserProfile& profile) noexcept;
		void Reset();

	protected:
		void OnSize() override;

	private:
		fig::observer_ptr<NonOwningImageWithMask> _pPortrait {};
		fig::sdl::Texture _portraitTexture {};

		fig::observer_ptr<StaticText> _pLabel {};
		fig::observer_ptr<ButtonWithIcon> _pSettingsButton {};
		fig::observer_ptr<ButtonWithIcon> _pSignOutButton {};
	};


}