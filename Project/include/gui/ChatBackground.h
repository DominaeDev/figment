#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class ChatBackground : public Control
	{
	public:
		ChatBackground(ParentPtr pParent);
		void SetTexture(TexturePtr pTexture) noexcept;
		void SetImage(const fig::uuid& assetId);
		void SetBrightness(float alpha);
		void SetBrightness(uint8_t alpha);
		void SetAlpha(float alpha);
		void SetAlpha(uint8_t alpha);

		void OnRender(RendererPtr pRenderer);
	
	private:
		Rectf GetImageRect() const;

	protected:
		TexturePtr _pTexture = nullptr;
		Point _imageSize {};
		float _fImageRatio {};
		uint8_t _value { 0xFF };
		uint8_t _alpha { 0xFF };
		ImageFit _fit { ImageFit::Outside };
	};
}