#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class TexturedBorder;
	class Image;

	using SliderValueChangedDelegate = std::function<void(float)>;

	class Slider : public Area
	{
	public:
		Slider(ControlPtr pParent);
		Slider(ControlPtr pParent, float fMin, float fMax);

		void SetValue(float value);
		float GetValue() const { return _value; }

		void SetDelegate(SliderValueChangedDelegate fnDelegate) { _fnDelegate = fnDelegate; }

	protected:
		void OnUpdate(float fElapsed) override;
		EventResult OnEvent(fig::event& event) override;
		void OnSize() override;
		void OnEnabled(bool bEnabled) override;

		void RefreshBar();
		fig::rect GetThumbRect() const noexcept;

	private:
		fig::observer_ptr<TexturedBorder> _pBar;
		fig::observer_ptr<TexturedBorder> _pBarBorder;
		fig::observer_ptr<TexturedBorder> _pFill;
		fig::observer_ptr<Image> _pThumbBorder;
		fig::observer_ptr<Image> _pThumb;

		bool _bInvalidBar { true };
		float _value {};
		fig::coord _thumbHalfSize;
		bool _bDragging {};
		bool _bHovering {};
		float _fMin { 0.0f };
		float _fMax { 1.0f };

		SliderValueChangedDelegate _fnDelegate;
	};
}