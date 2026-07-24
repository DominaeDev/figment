#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class RenderTargetControl : public Control
	{
	protected:
		RenderTargetControl(ControlPtr parent);

		void Render(fig::renderer_ptr pRenderer) override;
		void SetAlpha(float alpha);
		void SetAlpha(uint8_t alpha);
	
		virtual void OnRenderMask(fig::renderer_ptr pRenderer, fig::sdl::Texture& texture) {};

	private:
		fig::sdl::Texture _targetTexture;
		uint8_t _alpha { 0xFF };
		fig::point _lastSize { -1, -1 };
	};
}