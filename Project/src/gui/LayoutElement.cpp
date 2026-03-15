#include <pch.h>
#include "gui/LayoutElement.h"
#include "gui/Sizer.h"

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
		if (not _bLayoutEnabled or _bCulled)
			return;
		
		bool bLayout = _bInvalidLayout;
		_bInvalidLayout = false;

		if (bLayout)
		{
			if (bLayout and _pSizer)
				_pSizer->Layout();
		}

		// Update childen
		for (auto& child : _children)
			child->Layout();
		
		if (bLayout)
		{
			OnAfterLayout();
		}
	}

	void LayoutElement::LayoutNow()
	{
		_bInvalidLayout = true;
		Layout();
	}

	void LayoutElement::SetPosition(Coord x, Coord y)
	{
		SetPosition(Point { x, y });
	}

	void LayoutElement::SetX(Coord x)
	{
		SetPosition(Point { x, _localPosition.y });
	}

	void LayoutElement::SetY(Coord y)
	{
		SetPosition(Point { _localPosition.x, y });
	}

	void LayoutElement::SetPosition(Point position)
	{
		_localPosition = position;

		if (_pParent)
		{
			_rect.x = _localPosition.x + _pParent->GetAbsoluteX();
			_rect.y = _localPosition.y + _pParent->GetAbsoluteY();
		}
		else
		{
			_rect.x = position.x;
			_rect.y = position.y;
		}

		for (auto& child : _children)
			child->OnParentMoved();
	}

	void LayoutElement::OnParentMoved()
	{
		SetPosition(_localPosition);
	}

	void LayoutElement::SetSize(Coord width, Coord height)
	{
		SetSize(Point(width, height));
	}

	void LayoutElement::SetWidth(Coord width)
	{
		SetSize(Point(width, _rect.h));
	}

	void LayoutElement::SetHeight(Coord height)
	{
		SetSize(Point(_rect.w, height));
	}

	void LayoutElement::SetSize(Point size)
	{
		_rect.w = size.x;
		_rect.h = size.y;
		_bInvalidLayout = true;

		for (auto& child : _children)
			child->InvalidateLayout();

		OnSize();
	}

	void LayoutElement::SetRect(Coord x, Coord y, Coord width, Coord height)
	{
		SetRect(Rect { x, y, width, height });
	}

	void LayoutElement::SetRect(Rect rect)
	{
		_rect = rect;

		_localPosition.x = _rect.x - (_pParent ? _pParent->GetRect().x : 0);
		_localPosition.y = _rect.y - (_pParent ? _pParent->GetRect().y : 0);

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
			SetPosition(Point {
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

	void LayoutElement::AddChild(LayoutElement* pLayoutElement)
	{
		auto itFind = std::find(_children.cbegin(), _children.cend(), pLayoutElement);
		if (itFind != _children.cend())
			return; // Already added

		_children.push_back(pLayoutElement);
		pLayoutElement->SetParent(this);
		InvalidateLayout();

		OnAddedChild(pLayoutElement);
	}

	bool LayoutElement::RemoveChild(LayoutElement* pChild)
	{
		auto it = std::find(_children.begin(), _children.end(), pChild);
		if (it != _children.end())
		{
			OnRemovedChild(*it);
			(*it)->SetParent(nullptr);
			_children.erase(it);
			return true;
		}

		return false;
	}

	bool LayoutElement::RemoveChildren(bool destroy)
	{
		if (_children.empty())
			return false;

		for (auto& child : _children)
		{
			OnRemovedChild(child);

			if (destroy)
				delete child;
			else
				child->SetParent(nullptr);
		}
		_children.clear();

		if (_pSizer)
			_pSizer->Clear();
		return true;
	}

	void LayoutElement::SetSizer(Sizer* pSizer)
	{
		_pSizer.reset(pSizer);

		if (pSizer)
		{
			pSizer->SetOwner(this);
			InvalidateLayout();
		}
	}

	void LayoutElement::SetParent(LayoutElement* pParent)
	{
		_pParent = pParent;
		SetPosition(_localPosition);

		OnParent();
	}

	void LayoutElement::InvalidateLayout()
	{
		_bInvalidLayout = true;
	}

	void LayoutElement::InvalidateParentLayout(bool bRefreshImmediately) //! @hmm?
	{
		if (!_pSizer && _pParent != nullptr)
			_pParent->InvalidateParentLayout();
		else
		{
			if (bRefreshImmediately && _pSizer)
				_pSizer->Layout();
			else
				InvalidateLayout();
		}
	}

	void LayoutElement::MoveChildToTop(LayoutElement* pChild)
	{
		auto it = std::find(_children.begin(), _children.end(), pChild);
		if (it == _children.end())
			return;

		_children.erase(it);
		_children.push_back(pChild);
	}

	void LayoutElement::MoveChildToBottom(LayoutElement* pChild)
	{
		auto it = std::find(_children.begin(), _children.end(), pChild);
		if (it == _children.end())
			return;

		_children.erase(it);
		_children.insert(_children.begin(), pChild);
	}
}