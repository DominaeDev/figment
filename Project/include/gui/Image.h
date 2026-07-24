#pragma once

#include "Control.h"

namespace fig::gui
{
	class Image : public Control
	{
	public:
		Image(control_ptr pParent, fig::texture_ptr pTexture, fig::color tint = { 0xFF, 0xFF, 0xFF, 0xFF });
		Image(control_ptr pParent, Resource texture, fig::color tint = { 0xFF, 0xFF, 0xFF, 0xFF });
		void SetTexture(fig::texture_ptr pTexture, bool bResize = false);
		void SetTexture(Resource texture, bool bResize = false);
		inline bool HasTexture() const noexcept { return (bool)_pTexture; }
		fig::point GetTextureSize() const noexcept;

		inline void Rotate(double angle) { _angle = angle; }

	protected:
		void OnRender(fig::renderer_ptr pRenderer) override;

	protected:
		fig::texture_ptr _pTexture;
		double _angle {};
	};
}