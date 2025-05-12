#pragma once

#include "Types.h"
#include "LayoutElement.h"
#include <vector>

class Sizer : public LayoutElement
{
public:
	enum Flag : int {
		None	= 0,
		Expand	= 1 << 0,
		Up		= 1 << 1,
		Down	= 1 << 2,
		Left	= 1 << 3,
		Right	= 1 << 4,
		All		= Up | Down | Left | Right,
		Default = Expand,
	};

public:
	void Layout();

	void Add(Control* pFrame, int proportion = 0, int flags = Flag::Default, int border = 0);
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

	std::vector<LayoutInfo> _items;

	unsigned int GetCount() const { return static_cast<unsigned int>(_items.size()); }

	virtual void OnLayout(SDL_FRect rect) = 0;
	void Update(float fDeltaTime) override {}
};

