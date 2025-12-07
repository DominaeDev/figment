export module Area;

import Control;

/// <summary>
/// Render-less panel
/// </summary>

export 
{
	class Area : public Control
	{
	public:
		Area(Control* pParent) : Control(pParent)
		{
		}

	protected:
		void OnUpdate(float fDeltaTime) override {};
		void OnRender(Renderer* pRenderer) override {}
	};
}
