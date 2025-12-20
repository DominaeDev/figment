#pragma once

namespace fig::gui
{
	class IUpdateable
	{
	public:
		virtual ~IUpdateable() = default;
		virtual void Update(float fDeltaTime) = 0;
	};
}