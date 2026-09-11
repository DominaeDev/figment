#pragma once

#include "gui/ImageViewport.h"

namespace fig::gui
{
	class ResizeHandle;

	using ResizedDelegate = std::function<void(fig::coord)>;

	class ResizeableImageViewport : public ImageViewport
	{
	public:
		ResizeableImageViewport(ControlPtr pParent);
		void SetResizedDelegate(ResizedDelegate fnDelegate) { _fnDelegate = fnDelegate; }
	protected:
		void OnSize() override;

	private:
		fig::observer_ptr<ResizeHandle> _pResizeHandle;
		ResizedDelegate _fnDelegate;
	};
}