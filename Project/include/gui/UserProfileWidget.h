#pragma once

#include "gui/Panel.h"
#include "model/UserProfile.h"

namespace fig::user
{
	struct UserProfile;
};

namespace fig::gui
{
	class ImageWithMask;
	class StaticText;
	class ButtonWithIcon;

	class UserProfileWidget : public Area
	{
	public:
		UserProfileWidget(LayoutElement* pParent) noexcept;

		void SetUser(const fig::user::UserProfile& profile) noexcept;

	protected:
		void OnSize() override;

	private:
		ImageWithMask* _pImage;
		StaticText* _pLabel;
		ButtonWithIcon* _pButton;
	};


}