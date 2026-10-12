#include <pch.h>
#include "gui/ImageAlignmentWidget.h"
#include "gui/ButtonWithIcon.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	constexpr fig::coord kMargin = 4;

	ImageAlignmentWidget::ImageAlignmentWidget(ControlPtr pParent) : Control(pParent)
	{
		for (size_t i = 0uz; i < _buttons.size(); ++i)
			CreateButton(static_cast<ImageAlignment>(i));
		
		auto pBackground = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pBackground->SetColor(Color::ButtonDefaultBackground);
		auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pBorder->SetColor(Color::Border);

		SetAlignment({});
	}

	void ImageAlignmentWidget::CreateButton(ImageAlignment alignment)
	{
		auto pButton = CreateControl<ButtonWithIcon>(Resource::ICON_ALIGNMENT_DOT);
		pButton->SetDelegate([this, alignment] { SetAlignment(alignment); });
		pButton->SetTheme(ButtonTheme {
			.defaultColor	{ Color::Undefined, Color::Icon, Color::Undefined },
			.hoverColor		{ Color::ButtonHoverBackground, Color::Icon, Color::Undefined },
			.pressedColor	{ Color::ButtonPressedBackground, Color::Icon, Color::Undefined },
			.disabledColor	{ Color::Undefined, Color::IconDisabled, Color::Undefined },
		});
		_buttons[toUZ(alignment)] = pButton;
	}

	void ImageAlignmentWidget::OnSize()
	{
		fig::coord buttonWidth = (GetWidth() - (kMargin * 2)) / 3;
		fig::coord buttonHeight = (GetHeight() - (kMargin * 2)) / 3;
		for (auto& pButton : _buttons)
			pButton->SetSize(buttonWidth, buttonHeight);
		_buttons[(size_t)ImageAlignment::Center]	->SetPosition(kMargin + buttonWidth * 1, kMargin + buttonHeight * 1);
		_buttons[(size_t)ImageAlignment::NorthWest] ->SetPosition(kMargin + buttonWidth * 0, kMargin + buttonHeight * 0);
		_buttons[(size_t)ImageAlignment::North]		->SetPosition(kMargin + buttonWidth * 1, kMargin + buttonHeight * 0);
		_buttons[(size_t)ImageAlignment::NorthEast] ->SetPosition(kMargin + buttonWidth * 2, kMargin + buttonHeight * 0);
		_buttons[(size_t)ImageAlignment::West]		->SetPosition(kMargin + buttonWidth * 0, kMargin + buttonHeight * 1);
		_buttons[(size_t)ImageAlignment::East]		->SetPosition(kMargin + buttonWidth * 2, kMargin + buttonHeight * 1);
		_buttons[(size_t)ImageAlignment::SouthWest] ->SetPosition(kMargin + buttonWidth * 0, kMargin + buttonHeight * 2);
		_buttons[(size_t)ImageAlignment::South]		->SetPosition(kMargin + buttonWidth * 1, kMargin + buttonHeight * 2);
		_buttons[(size_t)ImageAlignment::SouthEast] ->SetPosition(kMargin + buttonWidth * 2, kMargin + buttonHeight * 2);
	}

	void ImageAlignmentWidget::SetAlignment(ImageAlignment align, bool bSilent)
	{
		_value = align;

		for (size_t i = 0uz; i < _buttons.size(); ++i)
		{
			if (static_cast<size_t>(align) == i)
				_buttons[i]->SetIcon(Resource::ICON_ALIGNMENT_DOT_FILLED);
			else
				_buttons[i]->SetIcon(Resource::ICON_ALIGNMENT_DOT);
		}

		if (not bSilent and _fnDelegate)
			_fnDelegate(align);
	}

}