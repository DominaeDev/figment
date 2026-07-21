#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class RenderTargetControl : public Control
	{
	protected:
		RenderTargetControl(ParentPtr parent);

		void Render(RendererPtr pRenderer) override;
		void SetAlpha(float alpha);
		void SetAlpha(uint8_t alpha);
	
		virtual void OnRenderMask(RendererPtr pRenderer, fig::sdl::Texture& texture) {};

	private:
		fig::sdl::Texture _targetTexture;
		uint8_t _alpha { 0xFF };
		Point _lastSize { -1, -1 };
	};
}