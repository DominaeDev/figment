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
		void OnUpdate(float fDeltaTime) override {};
		void OnRender(Renderer* pRenderer) override;

	private:
		Texture* _pTexture = nullptr;
	};
}