#include <pch.h>
#include "gui/Icons.h"
#include "gui/GUIUtility.h"

namespace fig::gui
{
	IconsTable Icons::_icons {};

	void Icons::Init(fig::renderer_ptr pRenderer)
	{
		for (auto& kvp : IconFileMapping)
			LoadTexture(pRenderer, kvp.first, kvp.second);
	}

	bool Icons::LoadTexture(fig::renderer_ptr pRenderer, Icon icon, fig::path filename)
	{
		if (auto try_surface = LoadImage(filename))
		{
			if (auto texture = CreateTexture(pRenderer, (*try_surface).get()))
			{
				_icons[static_cast<size_t>(icon)] = std::move(texture);
				return true;
			}
		}

		assert(false && "Failed to load icon");
		return false;
	}
}