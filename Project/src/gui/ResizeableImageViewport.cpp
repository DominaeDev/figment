#include <pch.h>
#include "gui/ResizeableImageViewport.h"
#include "gui/ResizeHandle.h"

namespace fig::gui
{
	ResizeableImageViewport::ResizeableImageViewport(ControlPtr pParent) : ImageViewport(pParent)
	{
		_pResizeHandle = CreateControl<ResizeHandle>(Direction::South);
		_pResizeHandle->EnableDrawHandle(false);
		_pResizeHandle->SetDelegate([this](fig::coord size) {
			size = std::clamp(((size + 10) / 20) * 20, 240, 640);
			if (size != GetHeight())
			{
				SetHeight(size);
				InvalidateParentLayout();

				if (_fnDelegate)
					_fnDelegate(size);
			}
		});
	}

	void ResizeableImageViewport::OnSize()
	{
		ImageViewport::OnSize();

		if (_pResizeHandle)
			_pResizeHandle->FillParent();
	}
}