#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class ChatBackground : public Control
	{
	public:
		ChatBackground(ControlPtr pParent);
		void SetImage(const fig::uuid& assetId);
		
		void SetAlpha(float alpha);
		float GetAlpha() const noexcept { return toF(_alpha) / 255.0f; }
		void SetBrightness(float alpha);
		float GetBrightness() const noexcept { return toF(_value) / 255.0f; }
		void SetSaturation(float saturation);
		float GetSaturation() const noexcept { return toF(_saturation) / 255.0f; }
		void SetBlur(float sigma);
		float GetBlur() const noexcept { return _fBlurSigma; }

		void SetFit(ImageFit fit) { _fit = fit; }
		ImageFit GetFit() const noexcept { return _fit; }

		void SetBounded(bool bBounded) { _bBounded = bBounded; }
		void SetBoundingRect(const fig::rect& rect) { _boundingRect = rect; }
		bool IsBounded() const noexcept { return _bBounded; }

	protected:
		void OnUpdate(float fElapsed);
		void OnRender(fig::renderer_ptr pRenderer);
	
	private:
		void ProcessImage();
		void Saturate();
		void Blur();

	protected:
		fig::rect _boundingRect {};
		bool _bBounded {};
		fig::sdl::Surface _surface {};
		fig::sdl::Texture _texture {};
		fig::sdl::Surface _processedSurface {};
		uint8_t _value { 0xFF };
		uint8_t _saturation { 0xFF };
		uint8_t _alpha { 0xFF };
		float _fBlurSigma {};
		bool _bDirty {};
		ImageFit _fit { ImageFit::Outside };
	};
}