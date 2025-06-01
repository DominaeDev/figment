#pragma once

#include "Types.h"
#include "IUpdateable.h"

class LayoutElement;

class Sizer : public IUpdateable
{
public:
	enum Flag : int {
		None	= 0,
		Expand	= 1 << 0,
		Top		= 1 << 1,
		Bottom	= 1 << 2,
		Left	= 1 << 3,
		Right	= 1 << 4,

		AlignLeft				= 1 << 10,
		AlignRight				= 1 << 11,
		AlignTop				= 1 << 12,
		AlignBottom				= 1 << 13,
		AlignCenterHorizontal	= 1 << 14,
		AlignCenterVertical		= 1 << 15,

		All		= Top | Bottom | Left | Right,
		Default = None,
	};

public:
	void Layout();
	void SetOwner(LayoutElement* pOwner);

	void Add(Control* pFrame, int proportion = 0, int flags = Flag::Default, int border = 0);
	void AddStretchSpacer();
	void Remove(Control* pFrame);
	void Reset();

protected:
	struct LayoutInfo
	{
		Control* pFrame;
		int prop = 0;
		int flags = Flag::None;
		int border = 0;
	};

	LayoutElement* _pOwner = nullptr;
	std::vector<LayoutInfo> _items;

	unsigned int GetCount() const { return static_cast<unsigned int>(_items.size()); }

	virtual void OnLayout(SDL_FRect rect) = 0;
	void Update(float fDeltaTime) override {}

};

