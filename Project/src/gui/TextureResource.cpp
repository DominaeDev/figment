#include <pch.h>
#include "gui/TextureResource.h"
#include "gui/Icons.h"

namespace fig::gui
{
	TextureResource::TextureResource(Icon icon) :
		_ptr { Icons::Get(icon) }
	{
	}
}