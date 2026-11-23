#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class ImageWithMask : public Control
	{
	public:
		ImageWithMask(ControlPtr pParent, const fig::sdl::Surface& surface, fig::texture_ptr pMask);
		
		void SetTexture(const fig::sdl::Surface& surface, fig::texture_ptr pMask, bool bResize = false);
		void SetMask(fig::texture_ptr pMask, fig::coord cornerSize = {});
		void SetDirty();

		fig::point GetTextureSize() const noexcept;
		void Clear();

	protected:
		void OnRender(fig::renderer_ptr pRenderer) override;
		void OnSize() override;
		void Redraw();

	protected:
		fig::texture_ptr _pMask;
		fig::sdl::Texture _texture;
		fig::sdl::Texture _targetTexture;
		fig::point _lastSize {};
		fig::coord _cornerSize {};

		bool _bDirty { true };
	};
}
