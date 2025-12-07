export module CustomRenderer;

import Types;
import Graphics;

export
{
	class CustomRenderer
	{
	public:
		virtual void Render(Renderer* pRenderer, Rectf rect) = 0;
		virtual ~CustomRenderer() = default;
	};
}