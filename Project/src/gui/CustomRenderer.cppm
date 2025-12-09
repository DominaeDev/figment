export module CustomRenderer;

import Common;
import GUI.GraphicTypes;

export
{
	class CustomRenderer
	{
	public:
		virtual void Render(Renderer* pRenderer, Rectf rect) = 0;
		virtual ~CustomRenderer() = default;
	};
}