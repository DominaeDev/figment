#include <pch.h>
#include "gui/Control.h"
#include "gui/Window.h"
#include "gui/GUIUtility.h"
#include "gui/Sizer.h"
#include "gui/CustomRenderer.h"

namespace fig::gui
{
	Control::Control(ControlPtr pParent)
	{
		_pParent = pParent;
	}

	Control::Control(ControlPtr pParent, Window* pHostWindow) : Control(pParent)
	{
		_renderContext = std::make_shared<ControlRenderContext>(ControlRenderContext {
			.pWindow = pHostWindow->GetSDLWindow().get(),
			.pRenderer = pHostWindow->GetSDLRenderer().get(),
			.pTextEngine = pHostWindow->GetSDLTextEngine().get(),
		});
	}

	void Control::Render(fig::renderer_ptr pRenderer)
	{
		if (not _bVisible or _bCulled)
			return;

		static fig::rect* s_pClippingRect = nullptr;
		fig::rect* lastClippingRect = s_pClippingRect;
		fig::rect clippingRect;
		fig::rect cullingRect = expand_rect(GetRect(), 64);

		if (_bClipping)
		{
			fig::rect rect = GetRect();
			if (s_pClippingRect)
				SDL_GetRectIntersection(s_pClippingRect, &rect, &clippingRect);
			else
				clippingRect = rect;

			if (SDL_SetRenderClipRect(pRenderer, &clippingRect))
				s_pClippingRect = &clippingRect;
		}

		// Draw this
		OnRender(pRenderer);

		// Draw children
		for (auto& child : _children)
		{
			auto renderable = dynamic_cast<Control*>(child); //! @todo: remove this cast
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
			}
		}

		if (_bClipping)
		{
			s_pClippingRect = lastClippingRect;
			SDL_SetRenderClipRect(pRenderer, s_pClippingRect);
		}

		DrawBorder(pRenderer);
		OnPostRender();
	}

	void Control::OnRender(fig::renderer_ptr pRenderer)
	{
		DrawBackground(pRenderer); //! @todo: Move
	}

	void Control::DrawBorder(fig::renderer_ptr pRenderer)
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

	fig::color Control::GetForegroundColor() const
	{
		if (!_foregroundColor.IsDefined())
		{
			auto frameParent = dynamic_cast<Control*>(_pParent.get());
			return frameParent ? frameParent->GetForegroundColor() : fig::color();
		}
		return _foregroundColor;
	}

	fig::color Control::GetBackgroundColor() const
	{
		if (!_backgroundColor.IsDefined())
		{
			auto parentControl = dynamic_cast<Control*>(_pParent.get());
			return parentControl ? parentControl->GetBackgroundColor() : fig::color();
		}
		return _backgroundColor;
	}

	void Control::DrawBackground(fig::renderer_ptr pRenderer)
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
			SDL_SetRenderDrawColor(pRenderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
			SDL_RenderFillRect(pRenderer, &rect);
		}
	}

	void Control::OnParent()
	{
		LayoutElement::OnParent();

		auto pParent = dynamic_cast<Control*>(_pParent.get());
		if (pParent)
		{
			_renderContext = pParent->GetRenderContext();

			if (!_foregroundColor.IsDefined())
				_foregroundColor = pParent->GetForegroundColor();
			if (!_backgroundColor.IsDefined())
				_backgroundColor = pParent->GetBackgroundColor();
		}
	}

	EventResult Control::ProcessEvent(fig::event& event)
	{
		if (_bCulled)
			return EventResult::Pass;

		EventResult result { EventResult::Pass };
		// Pass to children (in reverse order)
		for (auto it = _children.rbegin(); it != std::rend(_children); ++it)
		{
			Control* pControl = dynamic_cast<Control*>(*it);
			if (pControl)
			{
				result = std::max(result, pControl->ProcessEvent(event));
				if (result == EventResult::Handled)
					return EventResult::Handled;
			}
		}

		// Pass to self
		if (result != EventResult::Handled)
			return OnEvent(event);
		return result;
	}

	void Control::SetBackgroundRenderer(CustomRenderer* pRenderer)
	{
		_pBGRenderer.reset(pRenderer);
	}

	void Control::SetBorderRenderer(CustomRenderer* pRenderer)
	{
		_pBorderRenderer.reset(pRenderer);
	}

	void Control::ClearBackgroundRenderer()
	{
		_pBGRenderer.reset();
	}

	void Control::ClearBorderRenderer()
	{
		_pBorderRenderer.reset();
	}

	void Control::SetVisible(bool bVisible) noexcept
	{ 
		if (_bVisible != bVisible)
		{
			_bVisible = bVisible;
			OnVisibility(_bVisible);
		}
	}

	void Control::SetEnabled(bool bEnabled) noexcept
	{ 
		if (_bEnabled != bEnabled)
		{
			_bEnabled = bEnabled;
			OnEnabled(_bEnabled);
		}
	}

	void Control::SetMargins(fig::coord left, fig::coord top, fig::coord right, fig::coord bottom)
	{
		_marginLeft = left;
		_marginTop = top;
		_marginRight = right;
		_marginBottom = bottom;
	}

	void Control::SetMargins(fig::rect rect)
	{
		SetMargins(rect.x, rect.y, rect.w, rect.h);
	}

	fig::rect Control::GetClientRect() const noexcept
	{
		fig::rect clientRect = GetRect();
		clientRect.x += _marginLeft;
		clientRect.y += _marginTop;
		clientRect.w -= _marginLeft + _marginRight;
		clientRect.h -= _marginTop + _marginBottom;
		return clientRect;
	}

	fig::point Control::GetMousePos() const noexcept
	{
		float x, y;
		auto _ = SDL_GetMouseState(&x, &y);
		return fig::point { toI(x), toI(y) };
	}

	std::shared_ptr<Control::ControlRenderContext> Control::GetRenderContext()
	{
		if (_renderContext)
			return _renderContext;

		if (auto pControlParent = dynamic_cast<Control*>(_pParent.get()))
		{
			_renderContext = pControlParent->GetRenderContext();
			return _renderContext;
		}
		return nullptr;
	}

	fig::window_ptr Control::GetSDLWindow() 
	{
		if (!_renderContext)
			_renderContext = GetRenderContext();

		if (_renderContext)
			return _renderContext->pWindow;
		return nullptr;
	}

	fig::renderer_ptr Control::GetSDLRenderer()
	{ 
		if (!_renderContext)
			_renderContext = GetRenderContext();

		if (_renderContext)
			return _renderContext->pRenderer;
		return nullptr;
	}

	fig::text_engine_ptr Control::GetSDLTextEngine() 
	{ 
		if (!_renderContext)
			_renderContext = GetRenderContext();

		if (_renderContext)
			return _renderContext->pTextEngine;
		return nullptr;
	}
}