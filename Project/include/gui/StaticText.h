#pragma once

#include "ControlWithMargins.h"
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

class StaticText : public ControlWithMargins
{
public:
	StaticText(Control* pParent, string text, FontFace fontFace, double ptSize, bool bAutoSize = true);
	virtual ~StaticText();

	TTF_Font* GetFont() const { return _pFont; }

	void SetText(string text);
	void SetTextAndResize(string text, int& newWidth, int& newHeight);
	string GetText() const { return _text; }

	void SetAlignment(TextAlignment alignment) { _alignment = alignment; }

	void SetForegroundColor(Color color) override;
	void SetBackgroundColor(Color color) override;

protected:
	Rectf GetAlignedRect() const;
	void InvalidateText();

	void OnUpdate(float fDeltaTime) override;
	void OnRender(Renderer* pRenderer) override;
	void OnParent() override;

private:
	void DrawText(int& textWidth, int& textHeight);
	void ReleaseTexture();

	string _text;
	bool _bInvalidated = false;
	bool _bAutoSize = true;

	TTF_Font* _pFont = nullptr;
	Texture* _pTexture = nullptr;
	int _textWidth;
	int _textHeight;

	TextAlignment _alignment = TextAlignment::Default;
};