#include <pch.h>
#include "gui/LayoutElement.h"
#include "gui/Sizer.h"
#include <cassert>

namespace fig::gui
{
	LayoutElement::~LayoutElement()
	{
		for (auto& child : _children)
			delete child;
	}

	void LayoutElement::Update(float fElapsed)
	{
		if (_bCulled)
			return;

		Layout();

		// Update this
		OnUpdate(fElapsed);

		// Update children
		for (auto& child : _children)
			child->Update(fElapsed);
	}

	void LayoutElement::Layout()
	{
		if (!_bInvalidLayout or !_bLayoutEnabled or _bCulled)
			return;
		
		if (_pSizer)
		{
			_pSizer->Layout(GetSizerRect());
			OnAfterLayout();
		}
		_bInvalidLayout = false;
	}

	void LayoutElement::LayoutNow()
	{
		_bInvalidLayout = true;
		Layout();
	}

	void LayoutElement::SetPosition(fig::coord x, fig::coord y)
	{
		SetPosition(fig::point { x, y });
	}

	void LayoutElement::SetX(fig::coord x)
	{
		SetPosition(fig::point { x, _localPosition.y });
	}

	void LayoutElement::SetY(fig::coord y)
	{
		SetPosition(fig::point { _localPosition.x, y });
	}

	void LayoutElement::SetPosition(fig::point position)
	{
		_localPosition = position;

		if (_pParent)
		{
			_rect.x = _localPosition.x + _pParent->GetOriginX();
			_rect.y = _localPosition.y + _pParent->GetOriginY();
		}
		else
		{
			_rect.x = position.x;
			_rect.y = position.y;
		}

		for (auto& child : _children)
			child->OnParentMoved();
	}

	void LayoutElement::SetAbsolutePosition(fig::coord x, fig::coord y)
	{
		SetAbsolutePosition(fig::point { x, y });
	}

	void LayoutElement::SetAbsolutePosition(fig::point position)
	{
		_rect.x = position.x;
		_rect.y = position.y;

		if (_pParent)
		{
			_localPosition.x = _rect.x - _pParent->GetOriginX();
			_localPosition.y = _rect.y - _pParent->GetOriginY();
		}
		else
		{
			_localPosition.x = position.x;
			_localPosition.y = position.y;
		}

		for (auto& child : _children)
			child->OnParentMoved();
	}

	void LayoutElement::OnParentMoved()
	{
		SetPosition(_localPosition);
	}

	void LayoutElement::SetSize(fig::coord width, fig::coord height)
	{
		SetSize(fig::point(width, height));
	}

	void LayoutElement::SetWidth(fig::coord width)
	{
		SetSize(fig::point(width, _rect.h));
	}

	void LayoutElement::SetHeight(fig::coord height)
	{
		SetSize(fig::point(_rect.w, height));
	}

	void LayoutElement::SetSize(fig::point size)
	{
		_rect.w = size.x;
		_rect.h = size.y;
		_bInvalidLayout = true;

		for (auto& child : _children)
			child->InvalidateLayout();

		OnSize();
	}

	void LayoutElement::SetRect(fig::coord x, fig::coord y, fig::coord width, fig::coord height)
	{
		SetRect(fig::rect { x, y, width, height });
	}

	void LayoutElement::SetRect(fig::rect rect)
	{
		_rect = rect;

		_localPosition.x = _rect.x - (_pParent ? _pParent->GetOriginX() : 0);
		_localPosition.y = _rect.y - (_pParent ? _pParent->GetOriginY() : 0);

		for (auto& child : _children)
		{
			child->OnParentMoved();
			child->InvalidateLayout();
		}
		_bInvalidLayout = true;
		OnSize();
	}

	void LayoutElement::Center()
	{
		if (_pParent)
		{
			SetPosition(fig::point {
				.x = (_pParent->GetWidth() - GetWidth()) / 2,
				.y = (_pParent->GetHeight() - GetHeight()) / 2,
			});
		}
	}

	void LayoutElement::CenterHorizontally()
	{
		if (_pParent)
			SetX((_pParent->GetWidth() - GetWidth()) / 2);
	}

	void LayoutElement::CenterVertically()
	{
		if (_pParent)
			SetY((_pParent->GetHeight() - GetHeight()) / 2);
	}

	void LayoutElement::FillParent()
	{
		if (_pParent)
		{
			SetPosition(0, 0);
			SetSize(_pParent->GetSize());
		}
	}

	void LayoutElement::AddChild(control_ptr pLayoutElement)
	{
#ifdef _DEBUG
		auto itFind = std::find(_children.cbegin(), _children.cend(), pLayoutElement);
		if (itFind != _children.cend())
		{
			assert(false && "Already added");
			return; // Already added
		}
#endif
		_children.push_back(pLayoutElement);
		pLayoutElement->SetParent(this);
		InvalidateLayout();

		OnAddedChild(pLayoutElement);
	}

	bool LayoutElement::RemoveChild(control_ptr pChild)
	{
		auto it = std::find(_children.begin(), _children.end(), pChild);
		if (it != _children.end())
		{
			if (_pSizer)
				_pSizer->Remove(*it);
			OnRemovedChild(*it);
			(*it)->SetParent(nullptr);
			_children.erase(it);
			return true;
		}

		return false;
	}

	bool LayoutElement::DestroyChild(control_ptr pChild)
	{
		if (RemoveChild(pChild))
		{
			delete pChild;
			return true;
		}
		return false;
	}

	bool LayoutElement::RemoveChildren()
	{
		if (_children.empty())
			return false;

		for (auto& child : _children)
		{
			OnRemovedChild(child);
			child->SetParent(nullptr);
		}
		_children.clear();

		if (_pSizer)
			_pSizer->Clear();
		return true;
	}

	bool LayoutElement::DestroyChildren()
	{
		if (_children.empty())
			return false;

		for (auto& child : _children)
		{
			OnRemovedChild(child);
			delete child;
		}
		_children.clear();

		if (_pSizer)
			_pSizer->Clear();
		return true;
	}

	void LayoutElement::SetParent(control_ptr pParent)
	{
		_pParent = pParent;
		SetPosition(_localPosition);

		OnParent();
	}

	void LayoutElement::InvalidateLayout()
	{
		_bInvalidLayout = true;
	}

	void LayoutElement::InvalidateParentLayout(bool bRefreshImmediately)
	{
		if (!_pSizer && _pParent != nullptr)
			_pParent->InvalidateParentLayout();
		else
		{
			if (bRefreshImmediately && _pSizer)
				_pSizer->Layout(GetSizerRect());
			else
				InvalidateLayout();
		}
	}

	void LayoutElement::MoveChildToTop(control_ptr pChild)
	{
		auto it = std::find(_children.begin(), _children.end(), pChild);
		if (it == _children.end())
			return;

		_children.erase(it);
		_children.push_back(pChild);
	}

	void LayoutElement::MoveChildToBottom(control_ptr pChild)
	{
		auto it = std::find(_children.begin(), _children.end(), pChild);
		if (it == _children.end())
			return;

		_children.erase(it);
		_children.insert(_children.begin(), pChild);
	}
}