#pragma once

#include "gui/NonOwningImageWithMask.h"

namespace fig::gui
{
	class PortraitImage : public NonOwningImageWithMask
	{
	public:
		PortraitImage(ControlPtr pParent);

		void SetCharacter(const fig::uuid& characterId);
		void SetDefault();

	private:
		fig::texture_ptr _pDefaultImage;
		fig::sdl::Texture _portraitTexture;
	};
}