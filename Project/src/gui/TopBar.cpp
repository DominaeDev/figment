#include <pch.h>
#include "gui/TopBar.h"
#include "gui/ScrollPanel.h"
#include "gui/LineBorderRenderer.h"

namespace fig::gui
{
	TopBar::TopBar(ControlPtr pParent, fig::string_view title) : Panel(pParent)
	{
		SetSizer<HorizontalSizer>();
		SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		Initialize(title);
	}

	TopBar::TopBar(ControlPtr pParent, fig::string_view title, fig::observer_ptr<ScrollPanel> pScrollPanel) : Panel(pParent),
		_pScrollPanel { pScrollPanel }
	{
		SetSizer<HorizontalSizer>();
		SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		Initialize(title);
	}

	void TopBar::Initialize(fig::string_view title)
	{
		DestroyChildren();

		// Create title
		_pTitle = CreateControl<StaticText>("", FontFace::Italic, 24, false);
		_pTitle->SetX(52);
		_pTitle->SetHeight(Constants::GUI::SidePanel::HeaderHeight);
		_pTitle->SetAlignment(TextAlignment::LeftCenter);
		_pTitle->SetTextAndResize(title);
		GetSizer()->Add(_pTitle, -1, SizerFlag::AlignCenterVertical | SizerFlag::Left, 18);

		// Create shadow
		if (_pScrollPanel)
		{
			_pShadow = CreateControl<VerticalGradient>(Color::LineColor, Color::LineColor);
			_pShadow->SetY(GetHeight());
			_pShadow->SetHeight(28);
		}
	}

	void TopBar::SetTitle(fig::string_view text) noexcept
	{
		if (_pTitle)
			_pTitle->SetText(text);
	}

	void TopBar::OnSize()
	{
		if (_pShadow)
			_pShadow->SetWidth(GetWidth());
	}

	void TopBar::OnUpdate(float)
	{
		if (_pShadow and _pScrollPanel)
		{
			float fScrollY = toF(_pScrollPanel->GetScrollY());
			const float fMinScrollY = 4.0f;
			const float fScrollFadeDistance = 160.0f;

			float fAlpha = std::clamp((fScrollY - fMinScrollY) / fScrollFadeDistance, 0.0f, 1.0f);

			if (fAlpha > 0.0f and GetBorderRenderer() == nullptr)
				SetBorderRenderer<LineBorderRenderer>(Color::LineColor, Direction::South);
			else if (fAlpha == 0.0f and GetBorderRenderer() != nullptr)
				SetBorderRenderer(nullptr);

			_pShadow->SetColors(Color::LineColor.WithAlpha(0.5f * fAlpha), Color::LineColor.WithAlpha(0.0f));
			_pShadow->SetVisible(fAlpha > 0.0f);
			if (fAlpha > 0.0f)
				GetBorderRenderer()->SetColor(Color::LineColor.WithAlpha(fAlpha));
		}
	}
}