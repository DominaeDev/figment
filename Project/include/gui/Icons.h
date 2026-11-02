#pragma once
#include "gui/Icon.h"

namespace fig::gui
{
	class Icons
	{
	public:
		static void Init(fig::renderer_ptr pRenderer);

	private:
		static IconsTable _icons;

		friend class TextureResource;
		constexpr static fig::texture_ptr Get(Icon icon)
		{
			return _icons.at(static_cast<size_t>(icon)).get();
		}

		static bool LoadTexture(fig::renderer_ptr pRenderer, Icon icon, fig::path filename);

	};
}