#pragma once

#include "Control.h"
#include "gui/GUITypes.h"

namespace fig::gui
{
	class Image : public Control
	{
	public:
		Image(Control* pParent, Texture* pTexture);
		void SetTexture(Texture* pTexture);

	protected:
		void OnRender(Renderer* pRenderer) override;

	protected:
		Texture* _pTexture = nullptr;
	};
}