#pragma once

#include "Control.h"
#include "Fonts.h"
#include "Types.h"

enum HorizontalAlignment : unsigned short
{
	Left	= (1u << 0),
	Center	= (1u << 1),
	Right	= (1u << 2),
};

enum VerticalAlignment : unsigned short
{
	Top		= (1u << 3),
	Middle	= (1u << 4),
	Bottom	= (1u << 5),
};

enum TextAlignment : unsigned short
{
	Left_Top = HorizontalAlignment::Left | VerticalAlignment::Top,
	Left_Center = HorizontalAlignment::Left | VerticalAlignment::Middle,
	Left_Bottom = HorizontalAlignment::Left | VerticalAlignment::Bottom,
	Middle_Top = HorizontalAlignment::Center | VerticalAlignment::Top,
	Middle_Center = HorizontalAlignment::Center | VerticalAlignment::Middle,
	Middle_Bottom = HorizontalAlignment::Center | VerticalAlignment::Bottom,
	Right_Top = HorizontalAlignment::Right | VerticalAlignment::Top,
	Right_Center = HorizontalAlignment::Right | VerticalAlignment::Middle,
	Right_Bottom = HorizontalAlignment::Right | VerticalAlignment::Bottom,
	Default = Left_Top,
};

class StaticText : public Control
{
public:
	StaticText(Control* pParent, string text, FontFace fontFace, double ptSize);
	virtual ~StaticText();

	void SetText(string text);

	void SetAlignment(TextAlignment alignment) { _alignment = alignment; }

protected:
	SDL_FRect GetAlignedRect() const;
	void OnUpdate(float fDeltaTime) override;
	void OnRender(SDL_Renderer* pRenderer) override;
	void OnParent() override;

private:
	void ReleaseTexture();

	string _text {};
	bool _bInvalidated = false;

	TTF_Font* _pFont = nullptr;
	SDL_Texture* _pTexture = nullptr;
	int _textWidth;
	int _textHeight;

	TextAlignment _alignment = TextAlignment::Default;
};