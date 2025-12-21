#include <pch.h>
#include "gui/LayoutElement.h"
#include "gui/Sizer.h"

using namespace fig::gui;

LayoutElement::~LayoutElement()
{
	for (auto& child : _children)
		delete child;

	delete _pSizer;
}

void LayoutElement::Layout()
{
	// Update child positions
	for (auto& child : _children)
	{
		auto& rect = child->GetRect();
		auto position = child->GetAbsolutePosition();
		rect.x = position.x;
		rect.y = position.y;
		child->Layout();
	}

	if (_pSizer)
		_pSizer->Layout();
	_bInvalidLayout = false;

	OnAfterLayout();
}

void LayoutElement::SetRect(Rectf rect)
{
	SetPosition(rect.x, rect.y);
	SetSize(rect.w, rect.h);
	OnSize();
}

void LayoutElement::SetRect(float x, float y, float width, float height)
{
	SetPosition(x, y);
	SetSize(width, height);
	OnSize();
}

Pointf LayoutElement::GetAbsolutePosition() const
{
	if (_pParent)
	{
		auto position = _pParent->GetAbsolutePosition();
		position.x += _position.x;
		position.y += _position.y;
		return position;
	}

	return _position;
}

void LayoutElement::SetPosition(float x, float y)
{
	SetPosition(Pointf(x, y));
}

void LayoutElement::SetX(float x)
{
	SetPosition(Pointf(x, _position.y));
}

void LayoutElement::SetY(float y)
{
	SetPosition(Pointf(_position.x, y));
}

void LayoutElement::SetPosition(Pointf position)
{
	_position = position;
	position = GetAbsolutePosition();
	_rect = Rectf(position.x, position.y, _rect.w, _rect.h);
	OnSize();
}

void LayoutElement::SetSize(Pointf size)
{
	_size = size;
	_rect.w = size.x;
	_rect.h = size.y;
	OnSize();
}

void LayoutElement::SetSize(float width, float height)
{
	SetSize(Pointf(width, height));
}

void LayoutElement::SetWidth(float width)
{
	SetSize(Pointf(width, _size.y));
}

void LayoutElement::SetHeight(float height)
{
	SetSize(Pointf(_size.x, height));
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

void LayoutElement::SetSizer(Sizer* pSizer)
{
	if (_pSizer)
		delete _pSizer;
	_pSizer = pSizer;

	if (pSizer)
		pSizer->SetOwner(this);
	
	InvalidateLayout();
}

void LayoutElement::SetParent(LayoutElement* pParent)
{
	_pParent = pParent;
	OnParent();
}

void LayoutElement::OnSize()
{
	InvalidateLayout();
}

void LayoutElement::OnParent()
{
	SetPosition(_position);
}

void LayoutElement::Update(float fDeltaTime)
{
	OnUpdate(fDeltaTime);
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