#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class NonOwningImageWithMask : public Control
	{
	public:
		NonOwningImageWithMask(ControlPtr pParent, fig::texture_ptr pTexture = nullptr, fig::texture_ptr pMask = nullptr);

		void SetTexture(fig::texture_ptr pTexture, fig::texture_ptr pMask, bool bResize = false);
		bool HasTexture() const noexcept { return _pTexture != nullptr; }
		void SetDirty();

	protected:
		void OnRender(fig::renderer_ptr pRenderer) override;
		void OnSize() override;
		void Redraw();

	protected:
		fig::texture_ptr _pTexture;
		fig::texture_ptr _pMask;
		fig::sdl::Texture _targetTexture;
		fig::point _lastSize {};

		bool _bDirty { true };
	};
}
