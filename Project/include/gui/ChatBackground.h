#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class ChatBackground : public Control
	{
	public:
		ChatBackground(ParentPtr pParent);
		void SetImage(const fig::uuid& assetId);
		void SetBrightness(float alpha);
		void SetBrightness(uint8_t alpha);
		void SetSaturation(float saturation);
		void SetAlpha(float alpha);
		void SetAlpha(uint8_t alpha);
		void SetBlur(float sigma);

	protected:
		void OnUpdate(float fElapsed);
		void OnRender(RendererPtr pRenderer);
	
	private:
		void ProcessImage();
		void Saturate();
		void Blur();
		Rectf GetImageRect() const;

	protected:
		fig::sdl::Surface _surface {};
		fig::sdl::Texture _texture {};
		fig::sdl::Surface _processedSurface {};
		Point _imageSize {};
		float _fImageRatio {};
		uint8_t _value { 0xFF };
		uint8_t _saturation { 0xFF };
		uint8_t _alpha { 0xFF };
		float _fBlurSigma {};
		bool _bDirty {};
		ImageFit _fit { ImageFit::Portrait };
	};
}