#include <pch.h>
#include "gui/Control.h"
#include "gui/Window.h"
#include "gui/GUIUtility.h"
#include "gui/Sizer.h"
#include "gui/CustomRenderer.h"

using namespace fig::gui::util;

namespace fig::gui
{
	Control::Control(LayoutElement* pParent)
	{
		if (pParent)
			pParent->AddChild(this);
		_pParent = pParent;
	}

	Control::Control(LayoutElement* pParent, Window* pHostWindow) : Control(pParent)
	{
		_renderContext = ControlRenderContext {
			.pWindow = pHostWindow->GetSDLWindow().get(),
			.pRenderer = pHostWindow->GetSDLRenderer().get(),
			.pTextEngine = pHostWindow->GetSDLTextEngine().get(),
		};
	}

	Control::~Control()
	{
		delete _pBGRenderer;
		delete _pBorderRenderer;
	}

	void Control::Render(Renderer* pRenderer)
	{
		if (not _bVisible or _bCulled)
			return;

		static Rect* s_pClippingRect = nullptr;
		Rect* lastClippingRect = s_pClippingRect;
		Rect clippingRect;
		Rect cullingRect = expand_rect(GetRect(), 64);

		if (_bClipping)
		{
			Rect rect = GetRect();
			if (s_pClippingRect)
				SDL_GetRectIntersection(s_pClippingRect, &rect, &clippingRect);
			else
				clippingRect = rect;

			if (SDL_SetRenderClipRect(pRenderer, &clippingRect))
				s_pClippingRect = &clippingRect;
		}

		// Draw this
		OnRender(pRenderer);

		DrawBorder(pRenderer);

		// Draw children
		for (auto& child : _children)
		{
			auto renderable = dynamic_cast<Control*>(child);
			if (renderable)
			{
				if (_bCulling)
				{
					auto childRect = renderable->GetRect();
					bool bVisible = SDL_HasRectIntersection(&cullingRect, &childRect);
					if (child->IsCulled() == bVisible)
						child->Cull(!bVisible);
				}

				renderable->Render(pRenderer);
				renderable->OnPostRender();
			}
		}

		if (_bClipping)
		{
			s_pClippingRect = lastClippingRect;
			SDL_SetRenderClipRect(pRenderer, s_pClippingRect);
		}
	}

	void Control::OnRender(Renderer* pRenderer)
	{
		DrawBackground(pRenderer);
	}

	void Control::DrawBorder(Renderer* pRenderer)
	{
		// Custom renderer
		if (_pBorderRenderer)
		{
			_pBorderRenderer->Render(pRenderer, GetDrawRect());
			return;
		}

		if (!_borderColor.IsDefined())
			return;

		auto rect = to_rectf(GetRect());
		SDL_SetRenderDrawColor(pRenderer, _borderColor.r, _borderColor.g, _borderColor.b, _borderColor.a);
		SDL_RenderRect(pRenderer, &rect);
	}

	Color Control::GetForegroundColor() const
	{
		if (!_foregroundColor.IsDefined())
		{
			auto frameParent = dynamic_cast<Control*>(_pParent);
			return frameParent ? frameParent->GetForegroundColor() : Color();
		}
		return _foregroundColor;
	}

	Color Control::GetBackgroundColor() const
	{
		if (!_backgroundColor.IsDefined())
		{
			auto parentControl = dynamic_cast<Control*>(_pParent);
			return parentControl ? parentControl->GetBackgroundColor() : Color();
		}
		return _backgroundColor;
	}

	void Control::DrawBackground(Renderer* pRenderer)
	{
		// Custom renderer
		if (_pBGRenderer)
		{
			_pBGRenderer->Render(pRenderer, GetDrawRect());
			return;
		}

		auto bgColor = GetBackgroundColor();
		if (bgColor.IsDefined() && bgColor.a != 0)
		{
			auto rect = to_rectf(GetRect());
			SDL_SetRenderDrawColor(pRenderer, bgColor.r, bgColor.g, bgColor.b, SDL_ALPHA_OPAQUE);
			SDL_RenderFillRect(pRenderer, &rect);
		}
	}

	void Control::OnParent()
	{
		LayoutElement::OnParent();

		auto pParent = dynamic_cast<Control*>(_pParent);
		if (pParent)
		{
			_renderContext = pParent->_renderContext;

			if (!_foregroundColor.IsDefined())
				_foregroundColor = pParent->GetForegroundColor();
			if (!_backgroundColor.IsDefined())
				_backgroundColor = pParent->GetBackgroundColor();
		}
	}

	bool Control::ProcessEvent(Event& event)
	{
		if (not _bVisible)
			return false;

		if (OnEvent(event))
			return true;

		for (auto it = _children.begin(); it != std::end(_children); ++it)
		{
			Control* pControl = dynamic_cast<Control*>(*it);
			if (pControl && pControl->ProcessEvent(event))
				return true;
		}
		return false;
	}

	void Control::SetBackgroundRenderer(CustomRenderer* pCustom)
	{
		if (_pBGRenderer != nullptr)
		{
			delete _pBGRenderer;
			_pBGRenderer = nullptr;
		}
		_pBGRenderer = pCustom;
	}

	void Control::SetBorderRenderer(CustomRenderer* pCustom)
	{
		if (_pBorderRenderer != nullptr)
		{
			delete _pBorderRenderer;
			_pBorderRenderer = nullptr;
		}
		_pBorderRenderer = pCustom;
	}

	void Control::SetVisible(bool bVisible)
	{ 
		if (_bVisible != bVisible)
		{
			_bVisible = bVisible;
			OnVisibility();
		}
	}

	void Control::SetMargins(Coord left, Coord top, Coord right, Coord bottom)
	{
		_marginLeft = left;
		_marginTop = top;
		_marginRight = right;
		_marginBottom = bottom;
	}

	void Control::SetMargins(Rect rect)
	{
		SetMargins(rect.x, rect.y, rect.w, rect.h);
	}

	Rect Control::GetClientRect() const noexcept
	{
		Rect clientRect = GetRect();
		clientRect.x += _marginLeft;
		clientRect.y += _marginTop;
		clientRect.w -= _marginLeft + _marginRight;
		clientRect.h -= _marginTop + _marginBottom;
		return clientRect;
	}
}