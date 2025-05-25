#include "LayoutElement.h"
#include "Sizer.h"

LayoutElement::~LayoutElement()
{
	for (auto& child : _children)
		delete child;
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
}

void LayoutElement::SetRect(SDL_FRect rect)
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

SDL_FPoint LayoutElement::GetAbsolutePosition() const
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
	SetPosition(SDL_FPoint(x, y));
}

void LayoutElement::SetPosition(SDL_FPoint position)
{
	_position = position;
	position = GetAbsolutePosition();
	_rect = SDL_FRect(position.x, position.y, _rect.w, _rect.h);
	OnSize();
}

void LayoutElement::SetSize(SDL_FPoint size)
{
	_size = size;
	_rect.w = size.x;
	_rect.h = size.y;
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
	auto itFind = std::find(std::cbegin(_children), std::cend(_children), pLayoutElement);
	if (itFind == std::cend(_children))
	{
		_children.push_back(pLayoutElement);
		InvalidateLayout();
	}
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

void LayoutElement::InvalidateParentLayout()
{
	if (!_pSizer && _pParent != nullptr)
		_pParent->InvalidateParentLayout();
	else
		InvalidateLayout();
}