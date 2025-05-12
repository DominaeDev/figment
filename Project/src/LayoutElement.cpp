#include "LayoutElement.h"
#include "Sizer.h"
#include "Utility.h"

LayoutElement::~LayoutElement()
{
	for (auto& child : _children)
		delete child;
}

void LayoutElement::Layout()
{
	if (_pSizer)
		_pSizer->Layout();
	else
	{
		// Update child positions
		for (auto& child : _children)
		{
			auto& rect = child->GetRect();
			auto position = child->GetAbsolutePosition();
			rect.x = position.x;
			rect.y = position.y;
		}
	}
	_bInvalidLayout = false;
}

void LayoutElement::SetRect(SDL_FRect rect)
{
	SetPosition(rect.x, rect.y);
	SetSize(rect.w, rect.h);
	_bInvalidLayout = true;
	OnSize();
}

void LayoutElement::SetRect(float x, float y, float width, float height)
{
	SetPosition(x, y);
	SetSize(width, height);
	_bInvalidLayout = true;
	OnSize();
}

SDL_FPoint LayoutElement::GetAbsolutePosition() const
{
	if (_pParent)
	{
		auto position = SDL_FPoint { _pParent->_rect.x, _pParent->_rect.y };
		position.x += _position.x;
		position.y += _position.y;
		return position;
	}

	return _position;
}

void LayoutElement::SetPosition(float x, float y)
{
	SetPosition(SDL_FPoint(x, y));
}

void LayoutElement::SetPosition(SDL_FPoint position)
{
	_position = position;
	position = GetAbsolutePosition();
	_rect = SDL_FRect(position.x, position.y, _rect.w, _rect.h);
	_bInvalidLayout = true;
	OnSize();
}

void LayoutElement::SetSize(SDL_FPoint size)
{
	_size = size;
	_rect.w = size.x;
	_rect.h = size.y;
	_bInvalidLayout = true;
	OnSize();
}

void LayoutElement::SetSize(float width, float height)
{
	SetSize(SDL_FPoint(width, height));
}

void LayoutElement::SetWidth(float width)
{
	SetSize(SDL_FPoint(width, _size.y));
}

void LayoutElement::SetHeight(float height)
{
	SetSize(SDL_FPoint(_size.x, height));
}

void LayoutElement::AddChild(LayoutElement* pLayoutElement)
{
	pLayoutElement->SetParent(this);
	_children.push_back(pLayoutElement);
}

bool LayoutElement::RemoveChild(LayoutElement* frame)
{
	auto it = std::find(std::begin(_children), std::end(_children), frame);
	if (it != std::end(_children))
	{
		(*it)->SetParent(nullptr);
		_children.erase(it);
		return true;
	}
	return false;
}

void LayoutElement::SetSizer(Sizer* pSizer)
{
	if (_pSizer)
	{
		RemoveChild(_pSizer);
		delete _pSizer;
	}

	if (pSizer)
	{
		_pSizer = pSizer;
		AddChild(pSizer);
	}
	
	_bInvalidLayout = true;
}

void LayoutElement::SetParent(LayoutElement* pParent)
{
	_pParent = pParent;
	OnParent();
}

void LayoutElement::OnSize()
{
	_bInvalidLayout = true;
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
	if (_pSizer)
		_pSizer->Layout();
}